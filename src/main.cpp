#include "http_server.h"
#include <iostream>
#include <csignal>
#include <memory>

std::unique_ptr<HttpServer> g_server;

void signalHandler(int signal){
    std::cout<<"\n Received signal: " << signal
        << ", stoping server..." << std::endl;
    
    if(g_server){
        g_server->stop();
    }
}


int main(int argc, char* argv[]){
    std::cout<<"HPHS v1.0"<<std::endl;

    int port = 8080;
    int threads = 4;
    std::string www_root = "./www";

    if(argc >= 2){
        port = std::atoi(argv[1]);
        if(port <= 0 || port > 65535){
            std::cerr << "Invalid port: " << port << std::endl;
            std::cerr << "Usage: " << argv[0] << "[port] [thread] [www_root]" << std::endl;
            return 1;
        }
    }

    if(argc >= 3){
        threads = std::atoi(argv[2]);
        if(threads <= 0) {
            std::cerr<<"Invaild thread count" << std::endl;
            return 1;
        }
    }

    if(argc >= 4){
        www_root = argv[3];
    }

    signal(SIGINT, signalHandler); //Ctrl+C
    signal(SIGTERM, signalHandler); //kill

    try{
        g_server = std::make_unique<HttpServer>(port, threads, www_root);
        g_server->start();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;

}