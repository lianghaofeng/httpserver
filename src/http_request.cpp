#include "http_request.h"
#include <sstream>
#include <algorithm>

bool HttpRequest::parse(const std::string& buffer){
    /*
    HTTP请求格式
    请求行\r\n
    头部1\r\n
    头部2\r\n
    \r\n
    请求体
    */

    size_t pos = buffer.find("\r\n\r\n");
    if(pos == std::string::npos){
        return false;
    }

    std::string header_part = buffer.substr(0, pos);
    body_ = buffer.substr(pos + 4);

    // 解析请求行
    size_t line_end = header_part.find("\r\n");
    if (line_end == std::string::npos){
        return false;
    }
    
    std::string request_line = header_part.substr(0, line_end);
    if (!parseRequestLine(request_line)){
        return false;
    }

    // 解析请求头
    std::string headers = header_part.substr(line_end + 2);
    parseHeaders(headers);

    return true;
}

bool HttpRequest::parseRequestLine(const std::string& line){
    // METHOD PATH VERSION
    // GET /index.html HTTP/1.1

    std::istringstream iss(line);
    std::string method_str;

    if (!(iss >> method_str >> path_ >> version_)){
        return false;
    }

    if (method_str == "GET"){
        method_ = GET;
    } else if (method_str == "POST"){
        method_ = POST;
    } else if (method_str == "HEAD"){
        method_ = HEAD;
    } else if (method_str == "PUT"){
        method_ = PUT;
    } else if (method_str == "DELETE"){
        method_ = DELETE;
    } else {
        method_ = INVALID;
        return false;
    }

    if (path_.empty()){
        path_ = "/";
    }

    return true;
}


void HttpRequest::parseHeaders(const std::string& header_part){
    std::istringstream stream(header_part);
    std::string line;

    while (std::getline(stream, line)){
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        size_t pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, pos);
        std::string value = line.substr(pos + 1);

        value.erase(0, value.find_first_not_of("\t"));
        headers_[toLower(key)] = value;
    }
}

std::string HttpRequest::getHeader(const std::string& key) const {
    std::string lower_key = toLower(key);
    auto it = headers_.find(lower_key);
    if(it != headers_.end()){
        return it->second;
    }
    return "";
}

bool HttpRequest::keepAlive() const {
    std::string connection = getHeader("Connection");

    if(version_ == "HTTP/1.1"){
        if (connection == "close") return false;
        return true;
    } else {
        if (connection != "keep-alive") return false;
        return true; 
    }
}

std::string HttpRequest::methodString() const {
    switch (method_){
        case GET: return "GET";
        case POST: return "POST";
        case HEAD: return "HEAD";
        case PUT: return "PUT";
        case DELETE: return "DELETE";
        default: return "INVALID";
    }
}

std::string HttpRequest::toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}