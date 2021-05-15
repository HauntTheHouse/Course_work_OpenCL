#include "TCPSocket.h"
#include <iostream>
#include <cstring>


TCPSocket::TCPSocket(const int &domain, const int& type)
{
    socketFD = socket(domain, type, 0);
    if (socketFD == -1)
    {
        throw std::invalid_argument("Socket creation failed...");
    }
    else
    {
        std::cout << "Socket successfully created..." << std::endl;
    }
}

void TCPSocket::setSocketAddress(const int &type, const in_addr_t& address, const uint16_t& port)
{
    memset(&socketAddress, 0, sizeof(socketAddress));
    socketAddress.sin_family = type;                    // type of address (IPv4)
    socketAddress.sin_addr.s_addr = address;            // 4 bytes for address
    socketAddress.sin_port = port;                      // 2 bytes for port
}

void TCPSocket::bindSocket()
{
    if (bind(socketFD, (sockaddr*)&socketAddress, sizeof(socketAddress)))
    {
        throw std::invalid_argument("Socket bind failed...");
    }
    else
    {
        std::cout << "Socket successfully bound..." << std::endl;
    }
}

void TCPSocket::listenForClients(const int& countOfConnections) const
{
    if (listen(socketFD, countOfConnections))
    {
        throw std::invalid_argument("Listen failed...");
    }
    else
    {
        std::cout << "Server listening..." << std::endl;
    }
}

void TCPSocket::connectToServer()
{
    if (connect(socketFD, (sockaddr*)&socketAddress, sizeof(socketAddress)))
    {
        throw std::invalid_argument("Connection with the server failed...");
    }
    else
    {
        std::cout << "Connected to the server..." << std::endl;
    }
}


void TCPSocket::acceptSocket(const TCPSocket* server)
{
    socklen_t len = sizeof(socketAddress);
    socketFD = accept(server->socketFD, (sockaddr*)&socketAddress, &len);
    if (socketFD == -1)
    {
        throw std::invalid_argument("Server accept failed...");
    }
    else
    {
        std::cout << "Server accept the client..." << std::endl;
    }
}

//template <typename T>
//void TCPSocket::receiveMessage(T& msg, const int& len)
//{
//    recv(socketFD, &msg, len, 0);
//}
//
//template <typename T>
//void TCPSocket::sendMessage(const T& msg, const int& len)
//{
//    send(socketFD, &msg, len, 0);
//}