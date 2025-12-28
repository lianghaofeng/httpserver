#include "http_server.h"
#include "http_request.h"
#include "http_response.h"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fstream>
#include <sstream>


HttpServer::HttpServer(int port, int thread_count, const std::string& www_root):
    port_(port),
    listen_fd_(-1),
    epoll_fd_(-1),
    www_root_(www_root),
    running_(false),
    pool_(nullptr){
        
    pool_ = std::make_unique<ThreadPool>(thread_count);
}

HttpServer::~HttpServer(){
    stop();
}

void HttpServer::start(){
    std::cout<<"启动服务器 Starting server..." <<std::endl;
    listen_fd_ = createListenSocket();

    // socket
    if(listen_fd_ < 0){
        std::cerr << "Failed to create listen socket" <<std::endl;
        return;
    }

    // epoll
    epoll_fd_ = epoll_create1(0);
    if(epoll_fd_ < 0){
        std::cerr << "Failed to create epoll" <<std::endl;
        return;
    }

    // add socket to epoll
    if(!addToEpoll(listen_fd_, true)){
        std::cerr << "Failed to add listen socket to epoll"
            << std::endl;
        return;
    }

    running_ = true;

    std::cout << "Server listening on port: " << port_ << std::endl;
    std::cout << "WWW root: " << www_root_ <<std::endl;
    std::cout << "Thread pool size: " << pool_->size() << std::endl;

    struct epoll_event events[MAX_EVENTS];

    while(running_){
        int n = epoll_wait(epoll_fd_, events, MAX_EVENTS, 1000);

        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr<<"epoll_wait error: " << strerror(errno) <<std::endl;
            break;
        }

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            
            if(fd == listen_fd_) {
                handleNewConnection();
            } else {
                if (events[i].events & (EPOLLIN | EPOLLERR | EPOLLHUP)) {
                    pool_->enqueue([this, fd] {
                        handleClient(fd);
                    });
                }
            }
        }

    }
    std::cout << "Server Stopped." << std::endl;

}

void HttpServer::stop() {
    running_ = false;

    if (epoll_fd_ >= 0) {
        close(epoll_fd_);
        epoll_fd_ = -1;
    }

    if (listen_fd_ >= 0) {
        close(listen_fd_);
        listen_fd_ = -1;
    }

}

int HttpServer::createListenSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        std::cerr << "socket() error" << strerror(errno) << std::endl;
        return -1;
    }
    // 设置SO_REUSEADDR，允许重用地址
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        std::cerr << "setsockopt() error: " 
            << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // 设置非阻塞
    if(!setNonBlocking(sockfd)) {
        close(sockfd);
        return -1;
    }
    
    // 绑定地址和端口
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);

    if (bind(sockfd, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
        std::cerr << "bind() error: " << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // start listening
    if (listen(sockfd, SOMAXCONN) < 0 ) {
        std::cerr << "listen() error: " << strerror(errno) << std::endl; 
        close(sockfd);
        return -1;
    }

    return sockfd;

}

bool HttpServer::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if(flags < 0) {
        std::cerr << "fcntl error: " << strerror(errno) << std::endl;
        return false;
    }
    if(fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "fcntl(F_SETFL) error: " << strerror(errno) << std::endl;
        return false;
    }
    return true;

}

bool HttpServer::addToEpoll(int fd, bool enable_et) {
    struct epoll_event ev;
    ev.events = EPOLLIN;

    if(enable_et) {
        ev.events |= EPOLLET; //边缘触发模式
    }

    ev.data.fd = fd;

    if(epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev) < 0){
        std::cerr << "epoll_ctl(ADD) error: " << strerror(errno) << std::endl;
        return false;
    }

    return true;
}

void HttpServer::removeFromEpoll(int fd) {
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd, nullptr);
}

void HttpServer::handleNewConnection(){
    // 循环接受所有等待的连接--边缘触发
    while(true){
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(listen_fd_, (struct sockaddr*)&client_addr, &client_len);

        if(client_fd < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                //所有连接都已接受
                break;
            }
            std::cerr << "accept() error: " << strerror(errno) << std::endl;
            break;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        std::cout<<"New Connection: " << client_ip
            << ": " <<ntohs(client_addr.sin_port) << std::endl;

        if(!setNonBlocking(client_fd)){
            close(client_fd);
            continue;
        }

        if(!addToEpoll(client_fd, true)){
            close(client_fd);
            continue;
        }
        
    }
}

void HttpServer::handleClient(int client_fd) {
    std::string buffer;

    ssize_t bytes_read = readRequest(client_fd, buffer);

    if(bytes_read <= 0) {
        closeConnection(client_fd);
        return;
    }

    HttpRequest request;

    if(!request.parse(buffer)){
        HttpResponse response;
        response.setStatusCode(400);
        response.setBody("<html><body<h1>400 Bad Request</h1></body></html>");
        response.setContentType("text/html");
        response.setKeepAlive(false);

        sendResponse(client_fd, response.build());
        closeConnection(client_fd);

        return;
    }

    std::cout << request.methodString() << " " << request.path() << std::endl;

    HttpResponse response;

    if(request.method() == HttpRequest::GET || 
    request.method() == HttpRequest::HEAD) {
        serveStaticFile(request.path(), response);
    } else {
        response.setStatusCode(405);
        response.setBody("<html><body<h1>405 Method Not Allowed</h1></body></html>");
        response.setContentType("text/html");
    }   

    response.setKeepAlive(request.keepAlive());

    // bool success = sendResponse(client_fd, response.build());


    bool success = false;

    if(response.useSendfile()){
        // 1. 先发送HTTP头，普通write
        std::string headers_only = response.build();
        success = sendResponse(client_fd, headers_only);

        // 2. 再用sendfile发送文件内容
        if(success) {
            success = sendFileWithSendfile(
                client_fd,
                response.getSendfilePath(),
                response.getSendfileSize()
            );
        }

    } else {
        // 普通发送
        success = sendResponse(client_fd, response.build());
    }

    if(!success || !request.keepAlive()){
        closeConnection(client_fd);
    }

}

ssize_t HttpServer::readRequest(int fd, std::string &buffer){
    char read_buffer[BUFFER_SIZE];
    ssize_t total_bytes = 0;

    while(true) {
        ssize_t bytes = read(fd, read_buffer, sizeof(read_buffer));

        if(bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 数据已全部读取
                break;
            }
            std::cerr<<"read() error: " << strerror(errno) <<std::endl;
            return -1;
        } else if (bytes == 0){
            return 0;
        }

        buffer.append(read_buffer, bytes);
        total_bytes += bytes;
        
        //检查是否读到完整的请求头 
        if(buffer.find("\r\n\r\n")!=std::string::npos){
            break;
        }
    }
    
    return total_bytes;
}


bool HttpServer::sendResponse(int fd, const std::string& response) {
    size_t total_sent = 0;
    const char* data = response.c_str();
    size_t len = response.length();

    while(total_sent < len) {
        ssize_t sent = write(fd, data + total_sent, len - total_sent);
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK){
                // 缓冲区满，稍后重试
                usleep(1000);
                continue;
            }
            std::cerr<<"write() error: " << strerror(errno) << std::endl;
            return false;  
        }
        total_sent += sent;
    }
    return true;
}

void HttpServer::serveStaticFile(const std::string& path, HttpResponse& response){
    // 构建完整文件路径
    std::string filepath = www_root_ + path;

    // 如果路径以/结尾，添加index.html
    if(filepath.back() == '/'){
        filepath += "index.html";
    }

    // 获取文件信息
    struct stat file_stat;
    if(stat(filepath.c_str(), &file_stat) < 0){
        //文件不存在
        response.setStatusCode(404);
        response.setBody("<html><body<h1>404 Not Found</h1></body></html>");
        response.setContentType("text/html");
        return;
    }

    // 文件存在
    response.setStatusCode(200);
    response.setContentType(HttpResponse::getContentType(filepath));

#ifdef USE_SENDFILE
    // 零拷贝方式
    response.setSendFilePath(filepath, file_stat.st_size);
#else
    // 普通读写方式
    std::string content = readFile(filepath);
    response.setBody(content);
#endif

}

std::string HttpServer::readFile(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);

    if (!file.is_open()){
        return "";
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();

}

void HttpServer::closeConnection(int fd){
    removeFromEpoll(fd);
    close(fd);
    std::cout <<"Connection closed: fd = " << fd << std::endl;
}

bool HttpServer::sendFileWithSendfile(int client_fd, const std::string& filepath, off_t file_size) {
    int file_fd = open(filepath.c_str(), O_RDONLY);
    if(file_fd < 0){
        std::cerr << "open() error: " <<  strerror(errno) << std::endl;
        return false;
    }

    bool success = true;

    off_t offset = 0;
    ssize_t sent = 0;
    while(offset < file_size){
        sent = sendfile(client_fd, file_fd, &offset, file_size - offset);
        // std::cout<< "sendfile: " << sent << std::endl;
        if(sent < 0){
            if(errno == EAGAIN || errno == EWOULDBLOCK) {
                //发送缓冲区满，等待后重试
                usleep(1000);
                continue;
            }
            std::cerr << "sendfile() error: " << strerror(errno) << std::endl;
            success = false;
            break;
        }

        // 如果sent == 0，可能是文件结束或出错
        if(sent == 0 && offset < file_size) {
            std::cerr << "sendfile() returned 0 unexpectedly" << std::endl;
            success = false;
            break;
        }
    }

    close(file_fd);
    return success;

}

