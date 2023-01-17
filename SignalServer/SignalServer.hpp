#include "Semaphore.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <netinet/in.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>

class SignalServer
{
public:
	SignalServer() = default;
	explicit SignalServer(int port) : port(port) {};
	~SignalServer();

	void start();
	void start(int port_num);
	void stop();
	[[nodiscard]] int getPort() const;
	std::string read();
	std::string read(int64_t timeout);

private:
	void capture();
	void setPort(int port_num);

private:
	int port {};
	int len {};
	int sock_fd {};
	struct sockaddr_in serv_addr{}, client_addr {};
	char recv_buf[1024] {};
	int recv_num {};
	Semaphore capture_sem;
	std::thread recv_thread;
	volatile bool _running = false;
	std::queue<std::string> queue;

};
