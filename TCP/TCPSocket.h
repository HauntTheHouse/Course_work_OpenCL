#ifndef TCPSocket_h
#define TCPSocket_h

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// #include <netdb.h>

class TCPSocket
{
private:
    int socketFD;
    sockaddr_in socketAddress;
public:
    TCPSocket() = default;
    TCPSocket(const int &domain, const int& type);
    ~TCPSocket() { shutdown(socketFD, 2); }

    void setSocketAddress(const int &type, const in_addr_t& address, const uint16_t& port);
    void bindSocket();
    void listenForClients(const int& countOfConnections) const;
    void connectToServer();
    void acceptSocket(const TCPSocket* server);
    template <typename T>
    void receiveMessage(T& msg, const int& len) { recv(socketFD, &msg, len, 0); }
    template <typename T>
    void sendMessage(const T& msg, const int& len) { send(socketFD, &msg, len, 0); };
};
#endif // TCPSocket_h
