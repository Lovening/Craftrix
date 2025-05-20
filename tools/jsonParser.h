#ifndef __JSON_PARSER_H__
#define __JSON_PARSER_H__

#include <string>
#include <functional>
#include <vector>
#include <iostream>
#include <memory>
#include <algorithm>
#include "memory_ptr.h"
#include "memory/memoryPool.hpp"
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
    
        // bool processChar(char c) {
        //     // Handle escape sequences in strings
        //     if (_escaped) {
        //         _escaped = false;
        //         return false;
        //     }
            
        //     if (c == '\\' && _in_string) {
        //         _escaped = true;
        //         return false;
        //     }
            
        //     // Handle string delimiters
        //     if (c == '"' && !_escaped) {
        //         _in_string = !_in_string;
        //         return false;
        //     }
            
        //     // Skip processing structure characters if inside a string
        //     if (_in_string) {
        //         return false;
        //     }
            
        //     // Handle JSON structure characters
        //     if (c == '{') {
        //         _json_started = true;
        //         _brace_count++;
        //     } else if (c == '}') {
        //         if (_brace_count > 0) {
        //             _brace_count--;
        //             // Only complete if all braces and brackets are balanced
        //             if (_json_started && _brace_count == 0 && _bracket_count == 0) {
        //                 return true; // 找到完整的JSON
        //             }
        //         }
        //     } else if (c == '[') {
        //         _bracket_count++;
        //     } else if (c == ']') {
        //         if (_bracket_count > 0) {
        //             _bracket_count--;
        //         }
        //     }
            
        //     return false;
        // }
        
    
        bool processChar(char c) {
            // 处理转义字符的代码保持不变...
        
            // 处理JSON结构字符
            if (c == '{') {
                _json_started = true;  // 以大括号开始
                _brace_count++;
            } else if (c == '}') {
                if (_brace_count > 0) {
                    _brace_count--;
                    // 检查是否完成
                    if (_json_started && _brace_count == 0 && _bracket_count == 0) {
                        return true; // 找到完整的JSON
                    }
                }
            } else if (c == '[') {
                _json_started = true;  // 添加：以中括号开始也设置JSON开始标志
                _bracket_count++;
            } else if (c == ']') {
                if (_bracket_count > 0) {
                    _bracket_count--;
                    // 添加：检查是否完成
                    if (_json_started && _brace_count == 0 && _bracket_count == 0) {
                        return true; // 找到完整的JSON
                    }
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
                    // std::cout << "当前字符: " << _buffer.substr(0, i+1) <<"1"<<std::endl;
                    
                    // 跳过前导空白字符直到找到一个JSON开始
                    if (!_state_tracker.isStarted()) {
                        if (isspace(c)) {
                            i++;
                            continue;
                        }
                    }

                    if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {

                        size_t start = 0;

                        // 找到完整的JSON，提取并处理
                        std::string json = _buffer.substr(start, i + 1);
                        json.erase(std::remove_if(json.begin(), json.end(), ::isspace), json.end());
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


#if 1
// 环形缓冲区JSON解析器
class RingBufferJsonParser : public JsonParserBase {
    public:
        RingBufferJsonParser(JsonCallback json_callback, ErrorCallback error_callback = nullptr, size_t buffer_size = 8192)
            : JsonParserBase(std::move(json_callback), std::move(error_callback)), 
              _buffer(buffer_size), 
              _size(buffer_size) {

              }
        #if 0
        void addData(const std::string& data) override {
            
            for (char c : data) {
                _buffer[_tail] = c;
                _tail = (_tail + 1) % _size;
                std::cout << "tail = " << _tail << " head = " << _head << std::endl;
                
                // 缓冲区已满，需要扩展
                if (_tail == _head) {
                    resizeBuffer();
                }
                
                // 处理这个字符
                if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {
                    
                    // std::cout << "找到完整的JSON " << std::endl;
                    // 找到完整的JSON，提取并处理它
                    std::string json = extractJson();
                    // std::cout << "找到完整的JSON: " << json << std::endl;
                    processJson(json);
                    
                    // 重置状态追踪器
                    _state_tracker.reset();
                }
            }
        }
        #endif

        void addData(const std::string& data) override {
            for (char c : data) {
                // 先检查下一个位置是否会导致缓冲区已满
                if ((_tail + 1) % _size == _head) {
                    resizeBuffer();
                }
                
                _buffer[_tail] = c;
                _tail = (_tail + 1) % _size;
                
                // 处理这个字符
                if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {
                    std::string json = extractJson();
                    processJson(json);
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
            size_t bracket_count = 0;
            bool in_string = false;
            bool escaped = false;
            
            size_t i = _head;
            while (i != _tail) {
                char c = _buffer[i];
                
                // 修改：检测JSON开始（大括号或中括号）
                if (!found_start && (c == '{' || c == '[')) {
                    found_start = true;
                }
                
                if (found_start) {
                    json.push_back(c);
                    
                    // 处理字符串和转义的逻辑保持不变...
                    
                    if (!in_string) {
                        if (c == '{') {
                            brace_count++;
                        } else if (c == '}') {
                            brace_count--;
                            // 检查是否为对象结束
                            if (brace_count == 0 && bracket_count == 0) {
                                found_end = true;
                                _head = (i + 1) % _size;
                                break;
                            }
                        } else if (c == '[') {
                            bracket_count++;
                        } else if (c == ']') {
                            bracket_count--;
                            // 添加：检查是否为数组结束
                            if (brace_count == 0 && bracket_count == 0) {
                                found_end = true;
                                _head = (i + 1) % _size;
                                break;
                            }
                        }
                    }
                }
                
                i = (i + 1) % _size;
            }
            
            if (!found_end) {
                return ""; // 没有找到完整的JSON
            }
            
            return json;
        }
        // std::string extractJson() {
        //     std::string json;
        //     bool found_start = false;
        //     bool found_end = false;
        //     size_t brace_count = 0;
        //     bool in_string = false;
        //     bool escaped = false;
        //     std::cout << "[DEBUG] _buffer: \"";
        //     for (size_t j = 0; j < _buffer.size(); ++j) {
        //         std::cout << _buffer[j];
        //     }
        //     std::cout << "\"" << std::endl;
        //     size_t i = _head;
        //     while (i != _tail) {
        //         char c = _buffer[i];
                
        //         // std::cout <<"i = "<<i <<" "<<"" << _buffer[i]; 
        //         if (!found_start && c == '{') {
        //             found_start = true;
        //         }
                
        //         if (found_start) {
        //             json.push_back(c);
                    
        //             if (escaped) {
        //                 escaped = false;
        //             } else if (c == '\\') {
        //                 escaped = true;
        //             } else if (c == '"') {
        //                 in_string = !in_string;
        //             } else if (!in_string) {
        //                 if (c == '{') {
        //                     brace_count++;
        //                 } else if (c == '}') {
        //                     brace_count--;
        //                     if (brace_count == 0) {
        //                         found_end = true;
        //                         _head = (i + 1) % _size; // 更新头指针
        //                         break;
        //                     }
        //                 }
        //             }
        //         }
                
        //         i = (i + 1) % _size;
        //     }
            
        //     if (!found_end) {
        //         // 没有找到完整的JSON
        //         return "";
        //     }
            
        //     return json;
        // }
      #if 0  


        void resizeBuffer() {
            std::cout << "扩展缓冲区" << std::endl;
            size_t new_size = _size * 2;
            std::vector<char> new_buffer(new_size);
            
            // 计算当前缓冲区中的数据量
            // size_t data_size = (_tail >= _head) ? 
            //                    (_tail - _head) : 
            //                    (_size - _head + _tail);
            // std::cout << "当前数据量: " << data_size << std::endl;
            std::cout << "当前缓冲区大小: " << _size << std::endl;
            size_t data_size = (_tail + _size - _head)-1;
            std::cout << "当前数据量: " << data_size << std::endl;
            // 复制数据到新缓冲区的开始位置
            size_t idx = 0;
            for (size_t i = _head; i != _tail; i = (i + 1) % _size) {
                new_buffer[idx++] = _buffer[i];
            }
            // std::copy(_buffer.begin(), _buffer.end(), new_buffer.begin());
            // 更新buffer和指针
            _buffer = std::move(new_buffer);
            _head = 0;
            _tail = idx;
            _size = new_size;
        }
        #endif

        void resizeBuffer() {
            size_t new_size = _size * 2;
            std::vector<char> new_buffer(new_size);
            
            // 正确计算当前缓冲区中的数据量
            size_t data_size = (_tail >= _head) ? (_tail - _head) : (_size - _head + _tail);
            
            // 复制数据到新缓冲区的开始位置
            size_t idx = 0;
            size_t i = _head;
            while (i != _tail) {
                new_buffer[idx++] = _buffer[i];
                i = (i + 1) % _size;
            }
            
            // 更新buffer和指针
            _buffer = std::move(new_buffer);
            _head = 0;
            _tail = idx;
            _size = new_size;
        }
        std::vector<char> _buffer;   // 环形缓冲区
        size_t _size;                // 缓冲区大小
        size_t _head = 0;            // 头指针
        size_t _tail = 0;            // 尾指针
        JsonStateTtacker _state_tracker;   // 状态追踪器
};
#endif 


#if 0
class RingBufferJsonParser : public JsonParserBase {
    public:
        // Fixed-size chunk for memory pool allocation
        struct BufferChunk {
            static const size_t SIZE = 1024;
            char data[SIZE];
        };
        
        RingBufferJsonParser(JsonCallback json_callback, ErrorCallback error_callback = nullptr, size_t buffer_size = 8192)
            : JsonParserBase(std::move(json_callback), std::move(error_callback)),
              _chunk_pool(32) // Allocate 32 chunks at a time for efficiency
        {
            // Calculate initial number of chunks needed
            size_t num_chunks = (buffer_size + BufferChunk::SIZE - 1) / BufferChunk::SIZE;
            
            // Ensure at least one chunk
            if (num_chunks == 0) num_chunks = 1;
            
            // Allocate initial chunks from memory pool
            _chunks.reserve(num_chunks);
            for (size_t i = 0; i < num_chunks; ++i) {
                _chunks.push_back(_chunk_pool.construct());
            }
            
            _capacity = num_chunks * BufferChunk::SIZE;
        }
        
        ~RingBufferJsonParser() {
            // Free all chunks back to memory pool
            for (auto* chunk : _chunks) {
                _chunk_pool.destroy(chunk);
            }
        }
        
        void addData(const std::string& data) override {
            for (char c : data) {
                // Check if buffer is full
                if ((_tail + 1) % _capacity == _head) {
                    expandBuffer();
                }
                
                // Store character in the appropriate chunk
                size_t chunk_idx = _tail / BufferChunk::SIZE;
                size_t pos = _tail % BufferChunk::SIZE;
                _chunks[chunk_idx]->data[pos] = c;
                
                _tail = (_tail + 1) % _capacity;
                
                // Process character
                if (_state_tracker.processChar(c) && _state_tracker.isComplete()) {
                    std::string json = extractJson();
                    processJson(json);
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
        // Extract JSON from chunked buffer
        std::string extractJson() {
            std::string json;
            bool found_start = false;
            bool found_end = false;
            size_t brace_count = 0;
            size_t bracket_count = 0;
            bool in_string = false;
            bool escaped = false;
            
            size_t i = _head;
            while (i != _tail) {
                size_t chunk_idx = i / BufferChunk::SIZE;
                size_t pos = i % BufferChunk::SIZE;
                char c = _chunks[chunk_idx]->data[pos];
                
                if (!found_start && (c == '{' || c == '[')) {
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
                            if (brace_count == 0 && bracket_count == 0) {
                                found_end = true;
                                _head = (i + 1) % _capacity;
                                break;
                            }
                        } else if (c == '[') {
                            bracket_count++;
                        } else if (c == ']') {
                            bracket_count--;
                            if (brace_count == 0 && bracket_count == 0) {
                                found_end = true;
                                _head = (i + 1) % _capacity;
                                break;
                            }
                        }
                    }
                }
                
                i = (i + 1) % _capacity;
            }
            
            if (!found_end) {
                return ""; // No complete JSON found
            }
            
            return json;
        }
        
        // Expand buffer by allocating more chunks from memory pool
        void expandBuffer() {
            size_t old_capacity = _capacity;
            size_t old_chunks = _chunks.size();
            
            // Reserve space for new chunks
            _chunks.reserve(old_chunks * 2);
            
            // Allocate new chunks (double the buffer size)
            for (size_t i = 0; i < old_chunks; ++i) {
                _chunks.push_back(_chunk_pool.construct());
            }
            
            _capacity = _chunks.size() * BufferChunk::SIZE;
            
            // Reorganize data if needed
            if (_head > _tail) {
                // Data wraps around, need to reorganize
                std::vector<char> temp;
                temp.reserve(old_capacity);
                
                // Copy data from old buffer
                for (size_t i = _head; i < old_capacity; ++i) {
                    size_t chunk_idx = i / BufferChunk::SIZE;
                    size_t pos = i % BufferChunk::SIZE;
                    temp.push_back(_chunks[chunk_idx]->data[pos]);
                }
                
                for (size_t i = 0; i < _tail; ++i) {
                    size_t chunk_idx = i / BufferChunk::SIZE;
                    size_t pos = i % BufferChunk::SIZE;
                    temp.push_back(_chunks[chunk_idx]->data[pos]);
                }
                
                // Reset head and tail
                _head = 0;
                _tail = temp.size();
                
                // Copy data back to buffer
                for (size_t i = 0; i < temp.size(); ++i) {
                    size_t chunk_idx = i / BufferChunk::SIZE;
                    size_t pos = i % BufferChunk::SIZE;
                    _chunks[chunk_idx]->data[pos] = temp[i];
                }
            }
        }
        
        std::vector<BufferChunk*> _chunks;     // Array of chunk pointers
        size_t _capacity = 0;                  // Total capacity in bytes
        size_t _head = 0;                      // Head position
        size_t _tail = 0;                      // Tail position
        JsonStateTtacker _state_tracker;       // JSON state tracker
        MemoryPool<BufferChunk> _chunk_pool;   // Memory pool for chunk allocation
};

#endif
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