#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#include <string>
#include <functional>
#include <vector>
#include <iostream>
#include <memory>
#include "memory_ptr.h"
// #include <nlohmann/json.hpp>

class JsonStateTtacker{

    private:
        int _brace_count = 0;        // 大括号计数
        int _bracket_count = 0;      // 中括号计数
        bool _in_string = false;     // 是否在字符串内
        bool _escaped = false;       // 是否是转义字符
        bool _json_started = false;  // 是否已开始JSON
    public:
        void reset() {
            _brace_count = 0;
            _bracket_count = 0;
            _in_string = false;
            _escaped = false;
            _json_started = false;
        }

    // 处理单个字符，返回是否找到完整的JSON
    
        bool processChar(char c) {
            // Handle escape sequences in strings
            if (_escaped) {
                _escaped = false;
                return false;
            }
            
            if (c == '\\' && _in_string) {
                _escaped = true;
                return false;
            }
            
            // Handle string delimiters
            if (c == '"' && !_escaped) {
                _in_string = !_in_string;
                return false;
            }
            
            // Skip processing structure characters if inside a string
            if (_in_string) {
                return false;
            }
            
            // Handle JSON structure characters
            if (c == '{') {
                _json_started = true;
                _brace_count++;
            } else if (c == '}') {
                if (_brace_count > 0) {
                    _brace_count--;
                    // Only complete if all braces and brackets are balanced
                    if (_json_started && _brace_count == 0 && _bracket_count == 0) {
                        return true; // 找到完整的JSON
                    }
                }
            } else if (c == '[') {
                _bracket_count++;
            } else if (c == ']') {
                if (_bracket_count > 0) {
                    _bracket_count--;
                }
            }
            
            return false;
        }
        
    
    // 检查是否已开始JSON
    bool isStarted() const {
        return _json_started;
    }

    // 检查JSON是否完整
    bool isComplete() const {
        return _json_started && _brace_count == 0 && _bracket_count == 0;
    }
};

class JsonParserBase {
    public:
        using JsonCallback = std::function<void(const std::string&)>;
        using ErrorCallback = std::function<void(const std::string&)>;
        
        JsonParserBase(JsonCallback json_callback, ErrorCallback error_callback = nullptr)
        : _json_callback(std::move(json_callback)),
          _error_callback(std::move(error_callback)) {}

        virtual ~JsonParserBase() = default;

        virtual void addData(const std::string& data) = 0;

        // 清空内部缓冲区
        virtual void clear() = 0;

    protected:
        // 处理完整的JSON
        void processJson(const std::string& json) {
            if (json.empty()) return;
            
            try {
                // 解析JSON
                // nlohmann::json parsed_json = nlohmann::json::parse(json);
                
                // 调用回调函数处理解析后的JSON
                if (_json_callback) {
                    _json_callback(json);
                }
            } catch (const std::exception& e) {
                // 调用错误回调
                if (_error_callback) {
                    _error_callback(e.what());
                } else {
                    std::cerr << "JSON解析错误: " << e.what() << std::endl;
                }
            }
        }
        
        JsonCallback _json_callback;
        ErrorCallback _error_callback;
};

// 增量解析
class InCrementalJsonParser : public JsonParserBase {
    
    public:
        InCrementalJsonParser(JsonCallback json_callback, ErrorCallback error_callback = nullptr)
            : JsonParserBase(std::move(json_callback), std::move(error_callback))
            {}


            void addData(const std::string& data) override {
                _buffer.append(data);
                
                // Use while loop instead of for loop to have better control over index increments
                size_t i = _last_pos;
                while (i < _buffer.size()) {
                    char c = _buffer[i];
                    // std::cout << "当前字符: " << _buffer.substr(0, i+1) << std::endl;
                    
                    if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {
                        // 找到完整的JSON，提取并处理
                        std::string json = _buffer.substr(0, i + 1);
                        // std::cout << "找到完整的JSON: " << json << std::endl;
                        processJson(json);
                        
                        // 移除已处理的数据
                        _buffer.erase(0, i + 1);
                        // std::cout << "剩下的内容: " << _buffer.substr(0, _buffer.length()) << std::endl;
                        
                        // 重置状态和索引
                        _state_tracker.reset();
                        i = 0; // 重置索引
                        
                        // 跳过前导空白字符
                        while (i < _buffer.size() && isspace(_buffer[i])) {
                            i++;
                        }
                        
                        // 如果缓冲区为空或没有新的JSON开始，退出循环
                        if (i >= _buffer.size() || _buffer[i] != '{') {
                            std::cout << "没有找到更多的JSON对象" << std::endl;
                            break;
                        }
                        
                        // Don't increment i here - continue to process the current character
                    } else {
                        // Only increment i when we haven't found a complete JSON
                        i++; 
                    }
                }
                
                // 更新最后处理的位置
                _last_pos = i;
            }
#endif
        void clear() override {
            _buffer.clear();
            _last_pos = 0;
            _state_tracker.reset();
        }
    
    private:
        std::string _buffer; // 内部缓冲区
        size_t _last_pos = 0; // 上次处理的位置
        JsonStateTtacker _state_tracker; // 状态跟踪器

};


// 环形缓冲区JSON解析器
class RingBufferJsonParser : public JsonParserBase {
    public:
        RingBufferJsonParser(JsonCallback json_callback, ErrorCallback error_callback = nullptr, size_t buffer_size = 8192)
            : JsonParserBase(std::move(json_callback), std::move(error_callback)), 
              _buffer(buffer_size), 
              _size(buffer_size) {}
        
        void addData(const std::string& data) override {
            for (char c : data) {
                _buffer[_tail] = c;
                _tail = (_tail + 1) % _size;
                
                // 缓冲区已满，需要扩展
                if (_tail == _head) {
                    resizeBuffer();
                }
                
                // 处理这个字符
                if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {
                    // 找到完整的JSON，提取并处理它
                    std::string json = extractJson();
                    processJson(json);
                    
                    // 重置状态追踪器
                    _state_tracker.reset();
                }
            }
        }
        
        void clear() override {
            _head = 0;
            _tail = 0;
            _state_tracker.reset();
        }
    
    private:
        // 从环形缓冲区提取JSON
        std::string extractJson() {
            std::string json;
            bool found_start = false;
            bool found_end = false;
            size_t brace_count = 0;
            bool in_string = false;
            bool escaped = false;
            
            size_t i = _head;
            while (i != _tail) {
                char c = _buffer[i];
                
                if (!found_start && c == '{') {
                    found_start = true;
                }
                
                if (found_start) {
                    json.push_back(c);
                    
                    if (escaped) {
                        escaped = false;
                    } else if (c == '\\') {
                        escaped = true;
                    } else if (c == '"') {
                        in_string = !in_string;
                    } else if (!in_string) {
                        if (c == '{') {
                            brace_count++;
                        } else if (c == '}') {
                            brace_count--;
                            if (brace_count == 0) {
                                found_end = true;
                                _head = (i + 1) % _size; // 更新头指针
                                break;
                            }
                        }
                    }
                }
                
                i = (i + 1) % _size;
            }
            
            if (!found_end) {
                // 没有找到完整的JSON
                return "";
            }
            
            return json;
        }
        
        // 扩展环形缓冲区
        void resizeBuffer() {
            std::vector<char> new_buffer(_size * 2);
            
            // 复制数据到新缓冲区
            size_t idx = 0;
            for (size_t i = _head; i != _tail; i = (i + 1) % _size) {
                new_buffer[idx++] = _buffer[i];
            }
            
            _buffer = std::move(new_buffer);
            _head = 0;
            _tail = idx;
            _size = _buffer.size();
        }
        
        std::vector<char> _buffer;   // 环形缓冲区
        size_t _size;                // 缓冲区大小
        size_t _head = 0;            // 头指针
        size_t _tail = 0;            // 尾指针
        JsonStateTtacker _state_tracker;   // 状态追踪器
    };
    
    // JSON解析器工厂
class JsonParserFactory {
    public:
        enum class ParserType {
            INCREMENTAL,   // 增量式解析器
            RING_BUFFER    // 环形缓冲区解析器
        };
        
        // 创建JSON解析器
        static std::unique_ptr<JsonParserBase> createParser(
            ParserType type,
            JsonParserBase::JsonCallback json_callback,
            JsonParserBase::ErrorCallback error_callback = nullptr,
            size_t buffer_size = 8192) {
                
            switch (type) {
                case ParserType::INCREMENTAL:
                    return std::make_unique<InCrementalJsonParser>(
                        std::move(json_callback), std::move(error_callback));
                
                case ParserType::RING_BUFFER:
                    return std::make_unique<RingBufferJsonParser>(
                        std::move(json_callback), std::move(error_callback), buffer_size);
                
                default:
                    throw std::invalid_argument("无效的解析器类型");
            }
        }
    };
#endif // __JSON_PARSER_H__