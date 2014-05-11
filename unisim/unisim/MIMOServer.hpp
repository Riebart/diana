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
	friend void on_hangup_MIMOServer(MIMOServer* srv, int32_t c);

public:
	MIMOServer(void (*data_callback)(int32_t), void (*hangup_callback)(int32_t), int32_t port, int32_t backlog = 5);
	MIMOServer(void (*data_callback)(int32_t, void*), void* data_context, void (*hangup_callback)(int32_t, void*), void* hangup_context, int32_t port, int32_t backlog = 5);
	~MIMOServer();

	void start();
	void stop();
	void hungup(int32_t c);

private:
	void hangup(int32_t c);
	void on_hangup(int32_t c);

	int32_t server4, server6;
	bool running;
	int32_t port;
	int32_t backlog;
	void (*data_callback)(int32_t);
	void (*data_callback2)(int32_t, void*);
	void* data_context;
	void (*hangup_callback)(int32_t);
	void (*hangup_callback2)(int32_t, void*);
	void* hangup_context;
	std::vector<int> inputs;
	std::vector<int> hangups;
	std::thread server_thread;
	// Map client FDs to their asynchronous reader threads.
	/// @todo Is this even necessary? Network input is serial anyway...
	std::map<int32_t, SocketThread*> threadmap;
	std::mutex hangup_lock;
};

#endif
