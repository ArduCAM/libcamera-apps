#include "SignalServer.hpp"
#include <iostream>

SignalServer::~SignalServer() {
    stop();
}

void SignalServer::start() {
    if (this->_running)
        return;

    if ((sock_fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }
    int b_optval = 1;
    if (setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &b_optval, sizeof(b_optval))) {
        perror("setsockopt failed");
		exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);

    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    len = sizeof(serv_addr);

    // Bind the socket with the server address
    if (bind(sock_fd, (const struct sockaddr *) &serv_addr,
             sizeof(serv_addr)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    this->_running = true;
    this->recv_thread = std::thread(&SignalServer::capture, this);
}

void SignalServer::start(int port_num) {
    setPort(port_num);
    start();
}

void SignalServer::stop() {
    this->_running = false;
    shutdown(sock_fd, SHUT_RDWR);
    if (this->recv_thread.joinable())
        this->recv_thread.join();
    close(sock_fd);
    capture_sem.cleanup();
}

void SignalServer::capture() {
    while (_running) {
        recv_num = recvfrom(sock_fd,
                            recv_buf,
                            sizeof(recv_buf),
                            0,
                            (struct sockaddr *) &client_addr,
                            (socklen_t *) &len);

        if (recv_num < 0) {
            perror("error reception :");
            exit(EXIT_FAILURE);
        }
        recv_buf[recv_num] = '\0';
        queue.push(recv_buf);
        capture_sem.notify(0);
    }
    _running = false;
}

std::string  SignalServer::read() {
    if (queue.empty()) {
        return "";
    }

    auto arg = queue.front();
    queue.pop();
    return arg;
}

std::string  SignalServer::read(int64_t timeout) {
    capture_sem.wait_for(0, timeout);
    return read();
}

int SignalServer::getPort() const {
    return port;
}

void SignalServer::setPort(int port_num) {
    port = port_num;
}
