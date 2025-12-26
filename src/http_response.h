#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include <string>
#include <unordered_map>

/**
 * 
 * Status: HTTP/1.1 200 OK
 * Headers: Content-Type: text/html
 * Body
 * 
 * -200 OK
 * -404 Not Found
 * -500 Internal Server Error
 */

class HttpResponse {
public:
    HttpResponse():
        status_code_(200),
        status_message_("OK"),
        version_("HTTP/1.1"){
            setHeader("Server", "HPHS/1.0");
        }

    /**
     * @param code 状态码(200, 400, 500)
     */
    void setStatusCode(int code);
    
    /**
     * @param body
     */
    void setBody(const std::string& body) { body_ = body;}

    /**
     * @param key 响应头
     * @param value 响应值
     */
    void setHeader(const std::string& key, const std::string& value){
        headers_[key] = value;
    }

    /**
     * @param type (text/html, application/json)
     */
    void setContentType(const std::string& type){
        setHeader("Content-Type", type);
    }

    /**
     * set KeepAlive
     */
    void setKeepAlive(bool keep_alive) {
        if (keep_alive){
            setHeader("Connection", "keep-alive");
        } else {
            setHeader("Connection", "close");
        }
    }

    /**
     * 构建HTTP响应字符串
     * @return HTTP 响应
     */
    std::string build();

    /**
     * 根据文件名猜测Content类型
     * @param path 文件路径
     * @return MIME类型
     */

    static std::string getContentType(const std::string& path);
    
private:
    static std::string getStatusMessage(int code);

private:
    int status_code_;
    std::string status_message_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

};


#endif