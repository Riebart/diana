#ifndef MIMOSERVER_HPP
#define MIMOSERVER_HPP

//! @todo These can probably be moved to the cpp file, and just have prototypes here.
#include <map>
#include <vector>
#include <stdint.h>

#ifdef WIN32
#include <thread>
#include <mutex>
typedef std::mutex LOCK_T;
typedef std::thread THREAD_T;
#else
#include <pthread.h>
typedef pthread_rwlock_t LOCK_T;
typedef pthread_t THREAD_T;
#endif

class SocketThread;

class MIMOServer
{
	friend void* serve_SocketThread(void* sock);
	friend void* serve_MIMOServer(void* server);

	friend void on_hangup_MIMOServer(MIMOServer* srv, int32_t c);

public:
	MIMOServer(void(*data_callback)(int32_t), void(*hangup_callback)(int32_t), int32_t port, int32_t backlog = 5);
	MIMOServer(void(*data_callback)(int32_t, void*), void* data_context, void(*hangup_callback)(int32_t, void*), void* hangup_context, int32_t port, int32_t backlog = 5);
	~MIMOServer();

	void start();
	void stop();
	void hungup(int32_t c);

    // Also provide a few convenience socket reading functions
    static int64_t socket_read(int fd, char* buf, int64_t count);
    static int64_t socket_write(int fd, char* buf, int64_t count);

private:
	void hangup(int32_t c);
	void on_hangup(int32_t c);

	int32_t server4, server6;
	bool running;
	int32_t port;
	int32_t backlog;
	void(*data_callback)(int32_t);
	void(*data_callback2)(int32_t, void*);
	void* data_context;
	void(*hangup_callback)(int32_t);
	void(*hangup_callback2)(int32_t, void*);
	void* hangup_context;
	std::vector<int32_t> inputs;
	std::vector<int32_t> hangups;
	// Map client FDs to their asynchronous reader threads.
	//! @todo Is this even necessary? Network input is serial anyway...
	std::map<int32_t, SocketThread*> threadmap;

	THREAD_T server_thread;
	LOCK_T hangup_lock;

};

#endif
