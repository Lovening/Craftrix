# C++11 内存池测试指南

这个项目包含了一个高性能的C++11内存池实现及其GoogleTest测试套件。

## 文件说明

- `memory_pool.h` - 内存池实现
- `memory_pool_test.cpp` - GoogleTest测试代码
- `Makefile` - 构建和运行测试的配置文件

## 依赖项

- C++11兼容的编译器（GCC 4.8+或Clang 3.3+）
- GoogleTest框架

## 安装依赖

### Ubuntu/Debian

```bash
sudo apt-get install build-essential
sudo apt-get install libgtest-dev
sudo apt-get install cmake # 用于编译gtest
cd /usr/src/gtest
sudo cmake .
sudo make
sudo cp lib/*.a /usr/lib
```

### CentOS/RHEL

```bash
sudo yum install gcc-c++ make
sudo yum install gtest-devel
```

### macOS

```bash
brew install googletest
```

## 构建和运行测试

### 全部测试

```bash
make all       # 编译测试
make run_all   # 运行所有测试
```

### 调试模式测试

调试模式测试启用了内存泄漏检测和其他安全检查。

```bash
make test_debug   # 编译
make run_debug    # 运行
```

### 发布模式测试

发布模式测试优化了性能，禁用了调试功能。

```bash
make test_release   # 编译
make run_release    # 运行
```

### 性能测试

比较内存池与标准分配器的性能差异。

```bash
make benchmark
```

### 压力测试

进行高强度的内存分配和释放测试。

```bash
make stress_test
```

## 测试内容说明

测试套件验证了内存池的以下功能:

1. **基本功能测试** - 分配、构造、销毁和释放
2. **智能指针支持** - 使用`make_shared`方法创建智能指针
3. **构造和析构正确性** - 确保对象正确构造和析构
4. **内存分配溢出测试** - 验证超出容量时的行为
5. **预分配功能测试** - 测试`reserve`方法
6. **多线程测试** - 验证在多线程环境下的正确性和性能
7. **性能对比测试** - 与标准分配器进行性能比较
8. **内存泄漏检测** - 验证内存泄漏检测功能(仅调试模式)
9. **压力测试** - 大量循环分配和释放，验证稳定性

## 自定义测试

你可以使用GoogleTest过滤器运行特定测试:

```bash
./memory_pool_test_debug --gtest_filter=MemoryPoolTest.BasicFunctionality
```

## 性能调优

可以通过修改内存池参数来优化性能:

1. **每块对象数量** - 根据使用模式调整
2. **TLS开关** - 单线程场景可禁用TLS提高性能
3. **块预分配** - 减少运行时内存分配开销