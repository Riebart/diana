#ifndef MIMOSERVER_HPP
#define MIMOSERVER_HPP

#include <map>
#include <vector>
#include <thread>
#include <mutex>

class SocketThread;

class MIMOServer
{
	friend void serve_SocketThread(SocketThread* sock);
	friend void serve_MIMOServer(MIMOServer* server);
	friend void on_hangup_MIMOServer(MIMOServer* srv, int c);

public:
	MIMOServer(void (*data_callback)(int), void (*hangup_callback)(int), int port, int backlog = 5);
	MIMOServer(void (*data_callback)(int, void*), void* data_context, void (*hangup_callback)(int, void*), void* hangup_context, int port, int backlog = 5);
	~MIMOServer();

	void start();
	void stop();
	void hungup(int c);

private:
	void hangup(int c);
	void on_hangup(int c);

	int server4, server6;
	bool running;
	int port;
	int backlog;
	void (*data_callback)(int);
	void (*data_callback2)(int, void*);
	void* data_context;
	void (*hangup_callback)(int);
	void (*hangup_callback2)(int, void*);
	void* hangup_context;
	std::vector<int> inputs;
	std::vector<int> hangups;
	std::thread server_thread;
	// Map client FDs to their asynchronous reader threads.
	/// @todo Is this even necessary? Network input is serial anyway...
	std::map<int, SocketThread*> threadmap;
	std::mutex hangup_lock;
};

#endif
