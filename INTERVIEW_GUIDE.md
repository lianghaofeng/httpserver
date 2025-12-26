# HTTP服务器项目面试准备指南

## 项目概述

### 项目基本信息
- **项目名称**: HPHS (High Performance HTTP Server) v1.0
- **技术栈**: C++ 11/14
- **代码规模**: 约500行核心代码
- **项目类型**: 基于Linux的高性能HTTP静态文件服务器

### 一句话介绍
这是一个用C++实现的高性能HTTP服务器，采用Epoll边缘触发模式和线程池技术，支持静态文件服务和HTTP/1.1协议。

---

## 核心技术架构

### 1. 整体架构设计

```
客户端请求
    ↓
监听Socket (非阻塞)
    ↓
Epoll事件监听 (边缘触发ET模式)
    ↓
接受连接 → 添加到Epoll
    ↓
客户端事件触发 → 提交到线程池
    ↓
工作线程处理请求
    ↓
解析HTTP请求 → 读取文件 → 构建响应
    ↓
发送响应给客户端
```

### 2. 核心模块

#### 模块1: HttpServer (服务器核心)
**职责**:
- Socket创建和管理
- Epoll事件循环
- 连接管理
- 静态文件服务

**关键实现**:
- `createListenSocket()`: 创建监听socket，设置SO_REUSEADDR，绑定端口
- `start()`: Epoll事件循环，处理新连接和客户端请求
- `handleNewConnection()`: 边缘触发模式下循环接受所有连接
- `handleClient()`: 处理单个客户端请求（在线程池中执行）

#### 模块2: ThreadPool (线程池)
**职责**:
- 管理工作线程
- 任务队列管理
- 并发请求处理

**关键技术**:
- 使用`std::thread`创建固定数量工作线程
- `std::queue`存储待执行任务
- `std::mutex`保护任务队列
- `std::condition_variable`实现线程同步
- 模板函数`enqueue()`支持任意可调用对象

**实现细节**:
```cpp
// 支持lambda和函数对象
template<class F, class... Args>
auto enqueue(F&& f, Args&&... args)
    -> std::future<typename std::invoke_result<F, Args...>::type>
```

#### 模块3: HttpRequest (请求解析)
**职责**:
- 解析HTTP请求行（Method, Path, Version）
- 解析HTTP头部
- 支持Keep-Alive判断

**支持的HTTP方法**: GET, POST, HEAD, PUT, DELETE

**解析流程**:
1. 查找`\r\n\r\n`分割头部和body
2. 解析请求行（Method Path Version）
3. 逐行解析Headers，存入unordered_map
4. Header key转小写存储（不区分大小写）

#### 模块4: HttpResponse (响应构建)
**职责**:
- 构建HTTP响应
- 状态码管理
- Content-Type自动识别

**支持的MIME类型**:
- 文本: html, css, js, txt, xml
- 图片: jpg, png, gif, svg, ico
- 其他: pdf, zip, mp4, mp3

---

## 高频面试问题与答案

### Q1: 请介绍一下你的HTTP服务器项目

**回答要点**:
"我实现了一个基于C++的高性能HTTP静态文件服务器。项目使用了Linux的Epoll边缘触发模式实现IO多路复用，结合线程池实现并发请求处理。

核心特点包括：
1. **高性能IO模型**: 使用Epoll ET模式，单线程监听可处理大量并发连接
2. **并发处理**: 线程池处理实际的HTTP请求，避免频繁创建销毁线程
3. **非阻塞IO**: 所有socket都设置为非阻塞模式
4. **HTTP/1.1支持**: 完整解析HTTP请求，支持Keep-Alive长连接
5. **静态文件服务**: 自动识别MIME类型，支持多种文件格式

代码约500行，包含完整的HTTP协议解析、Epoll事件处理、线程池实现等核心功能。"

---

### Q2: 为什么选择Epoll？它和Select/Poll有什么区别？

**回答要点**:

**选择Epoll的原因**:
1. **性能优势**: Epoll时间复杂度O(1)，而select/poll是O(n)
2. **支持大量连接**: select有FD_SETSIZE限制（通常1024），epoll理论上无限制
3. **边缘触发模式**: 减少不必要的系统调用

**三者对比**:

| 特性 | select | poll | epoll |
|------|--------|------|-------|
| 最大连接数 | 受FD_SETSIZE限制（1024） | 无限制 | 无限制 |
| fd拷贝 | 每次调用都要拷贝 | 每次调用都要拷贝 | 只在添加时拷贝 |
| 时间复杂度 | O(n) | O(n) | O(1) |
| 触发模式 | 只支持LT | 只支持LT | 支持LT和ET |

**代码实现位置**: `http_server.cpp:40-51, 165-181`

---

### Q3: 什么是Epoll的边缘触发(ET)和水平触发(LT)？你为什么选择ET？

**回答要点**:

**LT (Level Triggered - 水平触发)**:
- 只要fd可读/可写，epoll_wait就会返回
- 类似select/poll的行为
- 编程简单，不容易遗漏事件

**ET (Edge Triggered - 边缘触发)**:
- 只在状态变化时通知（从不可读到可读）
- 必须一次性读完所有数据
- 性能更高，但编程复杂

**我选择ET的原因**:
1. **减少系统调用**: 避免重复通知，提高性能
2. **更高效**: 在高并发场景下表现更好
3. **配合非阻塞IO**: 可以一次性处理完所有数据

**ET模式的注意事项**:
```cpp
// 1. 必须设置非阻塞
setNonBlocking(fd);

// 2. 必须循环读取直到EAGAIN
while(true) {
    ssize_t bytes = read(fd, buffer, size);
    if(bytes < 0) {
        if(errno == EAGAIN || errno == EWOULDBLOCK) {
            break; // 数据读完了
        }
    }
}
```

**代码位置**: `http_server.cpp:165-181, 187-220, 270-298`

---

### Q4: 你的线程池是如何实现的？

**回答要点**:

**设计思路**:
1. **固定线程数**: 启动时创建固定数量工作线程（默认4个，可配置）
2. **任务队列**: 使用`std::queue`存储待执行任务
3. **同步机制**: mutex保护队列，condition_variable通知线程

**核心实现**:

```cpp
// 构造函数：创建工作线程
ThreadPool::ThreadPool(size_t threads) {
    for(size_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            for(;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    // 等待任务或停止信号
                    condition.wait(lock, [this]{
                        return stop || !tasks.empty();
                    });

                    if(stop && tasks.empty()) return;

                    task = std::move(tasks.front());
                    tasks.pop();
                }
                task(); // 执行任务
            }
        });
    }
}
```

**enqueue实现特点**:
- 使用模板支持任意可调用对象
- 返回`std::future`可获取任务执行结果
- 使用`std::packaged_task`包装任务

**优势**:
- 避免频繁创建销毁线程的开销
- 限制并发数，防止资源耗尽
- 线程复用，提高响应速度

**代码位置**: `thread_pool.h, thread_pool.cpp`

---

### Q5: HTTP请求是如何解析的？

**回答要点**:

**HTTP请求格式**:
```
GET /index.html HTTP/1.1\r\n
Host: localhost:8080\r\n
Connection: keep-alive\r\n
\r\n
[请求体]
```

**解析流程**:

1. **查找分隔符**: 查找`\r\n\r\n`分割头部和body
```cpp
size_t pos = buffer.find("\r\n\r\n");
```

2. **解析请求行**: 提取Method, Path, Version
```cpp
std::istringstream iss(line);
iss >> method_str >> path_ >> version_;
```

3. **解析Headers**: 逐行解析，key:value格式
```cpp
size_t pos = line.find(':');
std::string key = line.substr(0, pos);
std::string value = line.substr(pos + 1);
headers_[toLower(key)] = value; // key转小写
```

**Keep-Alive判断**:
- HTTP/1.1默认keep-alive，除非Connection: close
- HTTP/1.0默认close，除非Connection: keep-alive

**代码位置**: `http_request.cpp:5-137`

---

### Q6: 如何处理并发连接？数据竞争如何避免？

**回答要点**:

**并发模型**:
- **主线程**: 运行Epoll事件循环，只负责监听和接受连接
- **工作线程**: 线程池中的线程处理具体的HTTP请求

**分工明确**:
```cpp
// 主线程：监听新连接
if(fd == listen_fd_) {
    handleNewConnection();
}
// 主线程：将请求提交到线程池
else {
    pool_->enqueue([this, fd] {
        handleClient(fd); // 工作线程执行
    });
}
```

**避免数据竞争**:
1. **每个连接独立**: 每个client_fd由一个线程处理，不共享
2. **线程池内部同步**: 使用mutex保护任务队列
3. **无共享状态**: HttpRequest和HttpResponse都是局部对象
4. **只读共享**: www_root_等配置只读，无需加锁

**为什么不需要给client_fd加锁**:
- 每个fd同一时间只由一个线程处理
- 处理完后才可能被再次提交到线程池
- Epoll ET模式下，一个事件只通知一次

**代码位置**: `http_server.cpp:72-84`

---

### Q7: 如何处理Keep-Alive长连接？

**回答要点**:

**Keep-Alive的意义**:
- 复用TCP连接，避免频繁三次握手/四次挥手
- 减少延迟，提高性能
- HTTP/1.1默认支持

**实现机制**:

1. **判断是否Keep-Alive**:
```cpp
bool HttpRequest::keepAlive() const {
    if(version_ == "HTTP/1.1") {
        return getHeader("Connection") != "close";
    } else {
        return getHeader("Connection") == "keep-alive";
    }
}
```

2. **设置响应头**:
```cpp
response.setKeepAlive(request.keepAlive());
// 设置 Connection: keep-alive 或 close
```

3. **决定是否关闭连接**:
```cpp
bool success = sendResponse(client_fd, response.build());
if(!success || !request.keepAlive()) {
    closeConnection(client_fd); // 关闭连接
}
// 否则保持连接，等待下次请求
```

**注意事项**:
- Keep-Alive的fd仍在Epoll中监听
- 下次请求到来会再次触发EPOLLIN事件
- 需要正确设置Content-Length

**代码位置**: `http_server.cpp:262-266, http_request.cpp:110-120`

---

### Q8: 如何保证非阻塞IO正确读取数据？

**回答要点**:

**非阻塞读取的挑战**:
- read可能返回EAGAIN（数据还未到）
- 一次read可能读不完所有数据
- ET模式下必须读完所有数据

**我的实现**:
```cpp
ssize_t HttpServer::readRequest(int fd, std::string &buffer) {
    char read_buffer[BUFFER_SIZE];
    ssize_t total_bytes = 0;

    while(true) {
        ssize_t bytes = read(fd, read_buffer, sizeof(read_buffer));

        if(bytes < 0) {
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                break; // 数据读完了
            }
            return -1; // 读取错误
        } else if(bytes == 0) {
            return 0; // 连接关闭
        }

        buffer.append(read_buffer, bytes);
        total_bytes += bytes;

        // HTTP请求头读完判断
        if(buffer.find("\r\n\r\n") != std::string::npos) {
            break;
        }
    }
    return total_bytes;
}
```

**关键点**:
1. **循环读取**: while循环确保读完所有数据
2. **EAGAIN处理**: 正常退出条件，不是错误
3. **协议完整性**: 检查`\r\n\r\n`判断请求头是否完整
4. **缓冲区管理**: 使用std::string自动管理内存

**代码位置**: `http_server.cpp:270-298`

---

### Q9: 如果要支持POST请求和文件上传，需要做哪些改进？

**回答要点**:

**当前限制**:
- 只读取到`\r\n\r\n`，body部分未完整读取
- 未解析Content-Length
- 未处理multipart/form-data

**需要的改进**:

**1. 完整读取Body**:
```cpp
// 1. 解析Content-Length
int content_length = std::stoi(request.getHeader("Content-Length"));

// 2. 继续读取body直到达到content_length
while(buffer.size() < header_size + content_length) {
    ssize_t bytes = read(fd, read_buffer, sizeof(read_buffer));
    // ...
}
```

**2. 处理大文件上传**:
```cpp
// 使用流式处理，避免一次性读入内存
std::ofstream outfile("upload.bin", std::ios::binary);
while(received < content_length) {
    ssize_t bytes = read(fd, buffer, size);
    outfile.write(buffer, bytes);
    received += bytes;
}
```

**3. 解析multipart/form-data**:
- 解析boundary
- 分割各个part
- 提取文件名和内容

**4. 路由系统**:
```cpp
std::unordered_map<std::string, HandlerFunc> routes_;
routes_["/upload"] = handleUpload;
```

**其他优化**:
- 限制上传文件大小
- 支持断点续传
- 文件类型验证

---

### Q10: 你的项目如何进行性能优化？还有哪些可以改进的地方？

**回答要点**:

**已实现的优化**:

1. **Epoll ET模式**: 减少系统调用
2. **线程池**: 避免线程创建销毁开销
3. **非阻塞IO**: 提高并发能力
4. **Keep-Alive**: 复用TCP连接
5. **SO_REUSEADDR**: 快速重启服务器

**可以改进的地方**:

**1. 零拷贝技术**:
```cpp
// 使用sendfile直接从文件发送到socket
#include <sys/sendfile.h>
sendfile(client_fd, file_fd, nullptr, file_size);
// 避免数据在内核态和用户态之间拷贝
```

**2. 内存池**:
- 预分配HttpRequest/HttpResponse对象
- 减少频繁的内存分配释放

**3. 静态文件缓存**:
```cpp
std::unordered_map<std::string, std::string> file_cache_;
// 将热点文件缓存在内存中
```

**4. HTTP解析优化**:
- 使用状态机代替字符串查找
- 减少string拷贝，使用string_view

**5. 连接管理**:
- 设置超时机制，及时关闭空闲连接
- 使用定时器管理Keep-Alive超时

**6. 日志系统**:
- 异步日志，避免阻塞IO
- 使用内存缓冲批量写入

**7. 监控和统计**:
- QPS统计
- 响应时间分布
- 连接数监控

**8. 多进程架构**:
- 使用SO_REUSEPORT多进程监听同一端口
- 提高CPU利用率

**代码位置**: 优化空间在各个模块都有体现

---

### Q11: 遇到过什么技术难点？如何解决的？

**可以谈的难点**:

**难点1: Epoll ET模式数据读取不完整**

**问题**:
- 一开始没有循环读取，导致数据读不完整
- HTTP请求解析失败

**解决**:
```cpp
// 必须循环读取直到EAGAIN
while(true) {
    ssize_t bytes = read(fd, buffer, size);
    if(bytes < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        break; // 读完了
    }
}
```

**难点2: 边缘触发下的新连接接受**

**问题**:
- 只accept一次，后续连接丢失

**解决**:
```cpp
void handleNewConnection() {
    while(true) { // 循环accept
        int client_fd = accept(listen_fd_, ...);
        if(client_fd < 0) {
            if(errno == EAGAIN) break; // 全部接受完
        }
    }
}
```

**难点3: 线程池析构时的死锁问题**

**问题**:
- 析构时如果直接join可能死锁

**解决**:
```cpp
~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true; // 先设置停止标志
    }
    condition.notify_all(); // 唤醒所有线程
    for(auto& worker : workers)
        worker.join(); // 等待结束
}
```

---

### Q12: 如何测试你的HTTP服务器？

**回答要点**:

**1. 基本功能测试**:
```bash
# 启动服务器
./build/http_server 8080 4 ./www

# 浏览器访问
http://localhost:8080/

# curl测试
curl http://localhost:8080/index.html

# 测试Keep-Alive
curl -v http://localhost:8080/
```

**2. 并发测试**:
```bash
# 使用ab (Apache Bench)
ab -n 10000 -c 100 http://localhost:8080/

# 使用wrk
wrk -t4 -c100 -d30s http://localhost:8080/
```

**3. 压力测试**:
```bash
# 高并发连接测试
ab -n 100000 -c 1000 http://localhost:8080/index.html
```

**4. 异常测试**:
```bash
# 发送错误的HTTP请求
echo -e "INVALID REQUEST\r\n\r\n" | nc localhost 8080

# 发送不完整的请求
echo -e "GET / HTTP/1.1\r\n" | nc localhost 8080
```

**5. 性能分析**:
```bash
# 使用perf分析
perf record -g ./http_server
perf report

# 使用valgrind检查内存泄漏
valgrind --leak-check=full ./http_server
```

---

## 技术细节速查表

### 关键系统调用
| 系统调用 | 作用 | 代码位置 |
|---------|------|---------|
| socket() | 创建socket | http_server.cpp:107 |
| bind() | 绑定地址和端口 | http_server.cpp:134 |
| listen() | 开始监听 | http_server.cpp:141 |
| accept() | 接受新连接 | http_server.cpp:193 |
| epoll_create1() | 创建epoll实例 | http_server.cpp:40 |
| epoll_ctl() | 添加/删除fd | http_server.cpp:175,184 |
| epoll_wait() | 等待事件 | http_server.cpp:62 |
| read() | 读取数据 | http_server.cpp:275 |
| write() | 发送数据 | http_server.cpp:307 |
| fcntl() | 设置非阻塞 | http_server.cpp:152-157 |

### 关键数据结构
| 类型 | 用途 |
|------|------|
| struct epoll_event | Epoll事件结构 |
| struct sockaddr_in | Socket地址结构 |
| std::queue<std::function<void()>> | 线程池任务队列 |
| std::unordered_map<string, string> | HTTP头部存储 |
| std::unique_ptr | 智能指针管理资源 |

### 编译运行
```bash
# 编译（假设有Makefile）
g++ -std=c++11 -pthread -o build/http_server \
    src/main.cpp src/http_server.cpp \
    src/http_request.cpp src/http_response.cpp \
    src/thread_pool.cpp

# 运行
./build/http_server [port] [threads] [www_root]

# 示例
./build/http_server 8080 4 ./www
```

---

## 扩展问题准备

### 网络编程基础
- TCP三次握手和四次挥手过程
- TIME_WAIT状态的作用
- TCP拥塞控制算法
- nagle算法

### 并发编程
- 互斥锁、读写锁、自旋锁的区别
- 条件变量的使用场景
- 线程安全的实现方式
- C++11并发特性

### HTTP协议
- HTTP/1.0, 1.1, 2.0的区别
- HTTP状态码含义
- Cookie和Session的区别
- HTTPS的原理

### Linux系统编程
- 进程和线程的区别
- 虚拟内存管理
- 文件描述符的本质
- 信号处理机制

---

## 项目亮点总结（30秒电梯演讲）

"我实现了一个C++高性能HTTP服务器，核心采用Epoll边缘触发模式配合线程池架构。主线程运行Epoll事件循环处理IO多路复用，工作线程池负责HTTP请求解析和响应构建。

技术亮点包括：全非阻塞IO、Keep-Alive长连接支持、完整的HTTP/1.1协议解析。整个项目约500行代码，但涵盖了网络编程、并发控制、协议解析等核心技术点。

通过这个项目，我深入理解了Linux网络编程、高并发服务器设计、以及C++11的智能指针和并发库的使用。"

---

## 注意事项

1. **诚实**: 不要夸大项目功能，明确说明是静态文件服务器
2. **深度**: 准备好被深挖任何技术细节
3. **举一反三**: 能从项目引申到相关技术领域
4. **思考**: 准备好"如果让你改进"这类问题
5. **对比**: 了解Nginx、Apache等成熟服务器的实现

祝面试顺利！🚀
