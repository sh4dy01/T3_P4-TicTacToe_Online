#pragma once
#include <WinSock2.h>
#include <vector>

/// <summary>
/// A connection to a client.
/// </summary>
struct Connection
{
    std::string Address = "Unknown";
    unsigned int Port = 0;
    std::string GetName() const;

    std::string Receive() const;
    void Send(const std::string& data) const;
    void Kick() const;

    // Creates the event object and associate it with the socket.
    Connection(SOCKET socket);
private:
    friend class TcpIpServer;

    SOCKET Socket;
    WSAEVENT Event;

    bool IsNew = true;
    mutable bool ReadPending = false;
    mutable bool ClosePending = false;
};
/// <summary>
/// A pointer to a connection.
/// Do not store this pointer, it may become invalid.
/// </summary>
typedef Connection* ClientPtr;

/// <summary>
/// TCP/IP server.
/// </summary>
class TcpIpServer final
{
public:
    TcpIpServer();
    ~TcpIpServer();
    TcpIpServer(const TcpIpServer&) = delete;
    TcpIpServer& operator=(const TcpIpServer&) = delete;

    /// <summary>
    /// Open server and start listening.
    /// </summary>
    /// <param name="port">Port number to listen.</param>
    void Open(unsigned int port);
    /// <summary>
    /// Stop listening and close all connections.
    /// </summary>
    void Close();

    /// <summary>
    /// Accept new connections and check all clients for pending data and close requests.
    /// </summary>
    void CheckNetwork();
    /// <summary>
    /// Find a client that has just connected.
    /// </summary>
    /// <returns>A client that is new, or nullptr.</returns>
    ClientPtr FindNewClient();
    /// <summary>
    /// Find a client that has pending data.
    /// </summary>
    /// <returns>A client that has pending data, or nullptr.</returns>
    ClientPtr FindClientWithPendingData();
    /// <summary>
    /// Close all clients that are marked for closing.
    /// </summary>
    /// <param name="lastCallback">A callback that will be called for each client that is closed.</param>
    /// <returns>The number of connections that were closed.</returns>
    int CleanClosedConnections(std::function<void(ClientPtr)> lastCallback = nullptr);

    /// <summary>
    /// Return all connections.
    /// </summary>
    const std::vector<Connection>& GetConnections() { return m_Connections; }
    /// <summary>
    /// Return a connection by its name.
    /// </summary>
    ClientPtr GetClientByName(const std::string& name);

private:

    WSADATA m_WsaData;
    SOCKET m_ListenSocket;
    WSAEVENT m_AcceptEvent;

    std::vector<Connection> m_Connections;
};
