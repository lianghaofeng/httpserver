#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

// 控制是否使用sendfile零拷贝发送
#define USE_SENDFILE

// 控制是否使用work-stealing模式
#define USE_WORK_STEALING

#ifdef USE_WORK_STEALING
#include "work_stealing_pool.h"
#else
#include "thread_pool.h"
#endif

#include <string>
#include <memory>
#include <atomic>
#include <sys/epoll.h>

class HttpServer {
    public:
    /**
     * @param port
     * @param thread_count
     * @param www_root
     */

    HttpServer(int port, int thread_count = 4, const std::string& www_root = "./www");

    ~HttpServer();

    void start();

    void stop();

    private:

    /**
     * @return 监听socket的文件描述符
     */
    int createListenSocket();

    /**
     * set non-blocking
     */
    bool setNonBlocking(int fd);

    /**
     * @param fd
     * @param enable_et
     */
    bool addToEpoll(int fd, bool enable_et = true);

    /**
     * remove fd
     */
    void removeFromEpoll(int fd);

    /**
     * handle new connection
     */
    void handleNewConnection();

    /**
     * handle client request 
     * @param client_fd
     */
    void handleClient(int client_fd);

    /**
     * read http request
     * @param fd
     * @param buffer
     * @return 读取的字节数， -1表示错误， 0表示连接关闭
     */
    ssize_t readRequest(int fd, std::string& buffer);

    /**
     * send http response
     * @param fd
     * @param buffer
     * @return 是否发送成功
     */
    bool sendResponse(int fd, const std::string& response);

    /**
     * 处理静态文件请求
     * @param path
     * @param resposne
     */
    void serveStaticFile(const std::string& path, class HttpResponse& response);

    /**
     * Read File
     */
    std::string readFile(const std::string& filepath);


    /**
     * 使用sendfile发送文件（零拷贝）
     * @param client_fd
     * @param filepath
     * @param file_size
     * @return if_sucess
     */
    bool sendFileWithSendfile(int client_fd, const std::string& filepath, off_t file_size);

    /**
     * close connection
     */
    void closeConnection(int fd);

    private:
    int port_;
    int listen_fd_;
    int epoll_fd_;
    std::string www_root_;
    std::atomic<bool> running_;

#ifdef USE_WORK_STEALING
    std::unique_ptr<WorkStealingPool> pool_;
#else
    std::unique_ptr<ThreadPool> pool_;
#endif

    static const int MAX_EVENTS = 1024;
    static const int BUFFER_SIZE = 4096;

};



#endif
