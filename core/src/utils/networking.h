#pragma once
#include <stdint.h>
#include <string>
#include <vector>
#include <mutex>
#include <inttypes.h>
#include <memory>
#include <thread>
#include <condition_variable>

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#else
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#endif

namespace net {
#ifdef _WIN32
    typedef SOCKET Socket;
#else
    typedef int Socket;
#endif

    enum Protocol {
        PROTO_TCP,
        PROTO_UDP
    };

    struct ConnReadEntry {
        int count;
        uint8_t* buf;
        void (*handler)(int count, uint8_t* buf, void* ctx);
        void* ctx;
    };

    struct ConnWriteEntry {
        int count;
        uint8_t* buf;
    };

    class ConnClass {
    public:
        ConnClass(Socket sock);
        ~ConnClass();

        void close();
        bool isOpen();
        void waitForEnd();

        int read(int count, uint8_t* buf);
        bool write(int count, uint8_t* buf);
        void readAsync(int count, uint8_t* buf, void (*handler)(int count, uint8_t* buf, void* ctx), void* ctx);
        void writeAsync(int count, uint8_t* buf);

    private:
        void readWorker();
        void writeWorker();

        bool stopWorkers = false;
        bool connectionOpen = false;

        std::mutex readMtx;
        std::mutex writeMtx;
        std::mutex readQueueMtx;
        std::mutex writeQueueMtx;
        std::mutex connectionOpenMtx;
        std::mutex closeMtx;
        std::condition_variable readQueueCnd;
        std::condition_variable writeQueueCnd;
        std::condition_variable connectionOpenCnd;
        std::vector<ConnReadEntry> readQueue;
        std::vector<ConnWriteEntry> writeQueue;
        std::thread readWorkerThread;
        std::thread writeWorkerThread;

        Socket _sock;

    };

    typedef std::unique_ptr<ConnClass> Conn;

    struct ListenerAcceptEntry {
        void (*handler)(Conn conn, void* ctx);
        void* ctx;
    };

    class ListenerClass {
    public:
        ListenerClass(Socket listenSock);
        ~ListenerClass();

        Conn accept();
        void acceptAsync(void (*handler)(Conn conn, void* ctx), void* ctx);

        void close();
        bool isListening();

    private:
        void worker();

        bool listening = false;
        bool stopWorker = false;

        std::mutex acceptMtx;
        std::mutex acceptQueueMtx;
        std::condition_variable acceptQueueCnd;
        std::vector<ListenerAcceptEntry> acceptQueue;
        std::thread acceptWorkerThread;

        Socket sock;

    };

    typedef std::unique_ptr<ListenerClass> Listener;

    Conn connect(Protocol proto, std::string host, uint16_t port);
    Listener listen(Protocol proto, std::string host, uint16_t port);

#ifdef _WIN32
    extern bool winsock_init;
#endif
}