#ifndef __BUF_LIST_HPP__
#define __BUF_LIST_HPP__

#include <list>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <string>

template<typename T>
class BufList {
    public:
        BufList(size_t max_size = 100, const std::string& name = "") 
            : _max_size(max_size), _name(name) {}

        // 禁止拷贝
        BufList(const BufList&) = delete;
        BufList& operator=(const BufList&) = delete;

        // 支持移动
        BufList(BufList&&) = default;
        BufList& operator=(BufList&&) = default;

        void set_name(const std::string& name) {
            std::lock_guard<std::mutex> lock(_mtx);
            _name = name;
        }

        std::string get_name() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _name;
        }

        size_t size() const {
            std::lock_guard<std::mutex> lock(_mtx);
            return _buf.size();
        }

        void clear() {
            std::lock_guard<std::mutex> lock(_mtx);
            _buf.clear();
        }

        // 写入（阻塞/超时/非阻塞）
        // ms = 0: 不阻塞；<0：永久阻塞；>0 超时
        bool write(const T& value, int64_t ms = 0) {
            std::unique_lock<std::mutex> lock(_mtx);
            auto is_full = [&]() { return _buf.size() >= _max_size; };
            if (ms == 0) {
                if (is_full()) return false;
            } else if (ms > 0) {
                if (! _not_full.wait_for(lock, std::chrono::milliseconds(ms), [&]() { return !is_full(); })) {
                    return false;
                }
            } else {
                _not_full.wait(lock, [&]() { return !is_full(); });
            }
            _buf.emplace_back(value);
            _not_empty.notify_one();
            return true;
        }

        // 移动写入
        bool write(T&& value, int64_t ms = 0) {
            std::unique_lock<std::mutex> lock(_mtx);
            auto is_full = [&]() { return _buf.size() >= _max_size; };
            if (ms == 0) {
                if (is_full()) return false;
            } else if (ms > 0) {
                if (! _not_full.wait_for(lock, std::chrono::milliseconds(ms), [&]() { return !is_full(); })) {
                    return false;
                }
            } else {
                _not_full.wait(lock, [&]() { return !is_full(); });
            }
            _buf.emplace_back(std::move(value));
            _not_empty.notify_one();
            return true;
        }
    
        // 读取（阻塞/超时/非阻塞）
        // out: 读取到的数据
        // ms = 0: 不阻塞；<0：永久阻塞；>0 超时
        bool read(T& out, int64_t ms = 0) {
            std::unique_lock<std::mutex> lock(_mtx);
            auto is_empty = [&]() { return _buf.empty(); };
            if (ms == 0) {
                if (is_empty()) return false;
            } else if (ms > 0) {
                if (! _not_empty.wait_for(lock, std::chrono::milliseconds(ms), [&]() { return !is_empty(); })) {
                    return false;
                }
            } else {
                _not_empty.wait(lock, [&]() { return !is_empty(); });
            }
            out = std::move(_buf.front());
            _buf.pop_front();
            _not_full.notify_one();
            return true;
        }
    
        // 唤醒一个阻塞中的写操作
        void resume_writer() {
            _not_full.notify_one();
        }
    
        // 唤醒一个阻塞中的读操作
        void resume_reader() {
            _not_empty.notify_one();
        }

        // 打印内容（须重载<<支持T的打印）
        void print() const {
            std::lock_guard<std::mutex> lock(_mtx);
            size_t idx = 0;
            for (const auto& val : _buf) {
                std::cout << "Buf[" << _name << "] idx:" << idx++ << ", val:" << val << std::endl;
            }
        }

    private:
        mutable std::mutex _mtx;
        std::condition_variable _not_empty;
        std::condition_variable _not_full;
        std::list<T> _buf;
        size_t _max_size;
        std::string _name;
};

#endif // __BUF_LIST_HPP__