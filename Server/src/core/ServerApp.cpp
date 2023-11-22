#include "ServerApp.h"
#include "ConsoleHelper.h"

#define ERR_CLR Color::Red // Error color
#define WRN_CLR Color::Yellow // Warning color
#define SCS_CLR Color::LightGreen // Success color
#define STS_CLR Color::LightMagenta // Status color
#define INF_CLR Color::Gray // Information color
#define DEF_CLR Color::White // Default color
#define HASH_CLR(c) HshClr(c->GetName()) << c->GetName()
#define HASH_STRING_CLR(c) HshClr(c) << c
#define WEB_PFX INF_CLR << '[' << STS_CLR << "WEB" << INF_CLR << "] " // Web server prefix

void ServerApp::Init()
{
    if (!InitGameServer())
    {
        std::cout << ERR_CLR << "Aborting app initialization." << std::endl;
        return;
    }

    if (!InitWebServer())
    {
        std::cout << WEB_PFX << WRN_CLR << "Web server is unable to start, web interface will be disabled for this session." << std::endl;
    }

    auto addr = TcpIp::IpAddress::GetLocalAddress();
    std::cout << INF_CLR << "=> Game Server Phrase: " << SCS_CLR << addr.ToPhrase() << std::endl;
    std::cout << INF_CLR << "=> Web Server Address: " << SCS_CLR << "http://" << addr.ToString() << ':' << DEFAULT_PORT + 1 << "/" << DEF_CLR << std::endl;
}

void ServerApp::Run()
{
    std::cout << INF_CLR << "Press ESC to shutdown the app." << std::endl << DEF_CLR << std::endl;
    while (!IsKeyPressed(27)) // 27 = ESC
    {
        HandleGameServer();
        HandleWebServer();

        // Sleep for 1ms to avoid 100% CPU usage
        Sleep(1);
    }
}

void ServerApp::CleanUp()
{
    std::cout << std::endl << INF_CLR << "User requested to shutdown the app." << std::endl << DEF_CLR;

    CleanUpWebServer();
    CleanUpGameServer();
}

#pragma region Game Server

bool ServerApp::InitGameServer()
{
    std::cout << Color::Cyan << "=========== Starting Game Server Initialization ===========" << std::endl << INF_CLR;
    try
    {
        m_GameServer = new TcpIpServer();
        m_GameServer->Open(DEFAULT_PORT);
        std::cout << "Game server is listening on port " << DEFAULT_PORT << "..." << std::endl;
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << ERR_CLR << "The game server is unable to start: " << e.what() << std::endl << DEF_CLR;
        return false;
    }

    std::cout << "Creating lobbies..." << std::endl;
    CreateLobbies();
    std::cout << "Lobbies created." << std::endl;

    std::cout << Color::Cyan << "=========== Game Server Initialization Complete ===========" << std::endl << DEF_CLR;
    return true;
}

void ServerApp::HandleGameServer()
{
    try
    {
        // Check for new connections / pending data / closed connections
        m_GameServer->CheckNetwork();

        // For each new connection
        ClientPtr newClient;
        while ((newClient = m_GameServer->FindNewClient()) != nullptr)
        {
            std::cout << STS_CLR << "New connection from " << HASH_CLR(newClient) << STS_CLR << " has been established." << std::endl << DEF_CLR;
        }

        // For each client with pending data
        ClientPtr sender;
        while ((sender = m_GameServer->FindClientWithPendingData()) != nullptr)
        {
            HandleRecv(sender);
        }

        // For each closed connection
        m_GameServer->CleanClosedConnections([](ClientPtr c)
            {
                std::cout << STS_CLR << "Connection from " << HASH_CLR(c) << STS_CLR << " has been closed." << std::endl << DEF_CLR;
            });
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << ERR_CLR << "The game server has encountered an error: " << e.what() << std::endl << DEF_CLR;
    }
}

void ServerApp::HandleRecv(ClientPtr sender)
{
    std::string data = sender->Receive();
    Json receivedData;
    try
    {
        receivedData = Json::parse(data);
    }
    catch (const std::exception& e)
    {
        std::cout << ERR_CLR << "Failed to parse JSON from " << HASH_CLR(sender) << ERR_CLR << ": " << e.what() << std::endl << DEF_CLR;
        return;
    }
    if (!receivedData.contains("Type"))
    {
        std::cout << ERR_CLR << "Received JSON from " << HASH_CLR(sender) << ERR_CLR << " does not contain a type." << std::endl << DEF_CLR;
        return;
    }

    if (receivedData["Type"] == "Login")
    {
        m_Players.insert({sender->GetName(), receivedData["UserName"]});
        std::cout << STS_CLR << "Registered player: " << HASH_STRING_CLR(receivedData["UserName"]) << STS_CLR << " into server." << std::endl << DEF_CLR;
    }
    ////////// Lobby State //////////
    else if (receivedData["Type"] == "GetLobbyList")
    {
        std::cout << INF_CLR << "Sending lobbies list to " << HASH_CLR(sender) << std::endl << DEF_CLR;
        SerializeLobbiesToJson(sender);
    }
    else if (receivedData["Type"] == "JoinLobby")
    {
        const int lobbyID = receivedData["ID"];

        for (auto lb : m_Lobbies)
        {
            if (lobbyID != lb->ID) continue;

            if (lb->IsInLobby(m_Players[sender->GetName()]) || lb->IsLobbyFull())
                return;

            const std::string& playerName = m_Players[sender->GetName()];

            lb->AddPlayerToLobby(playerName);
            std::cout << STS_CLR << "Player " << HASH_STRING_CLR(playerName) << STS_CLR << " has joined lobby: " << INF_CLR << lobbyID << std::endl << DEF_CLR;

            if (!m_StartedGames.contains(lb->ID))
            {
                m_StartedGames.insert({ lb->ID, lb });
                std::cout << STS_CLR << "Creating game " << INF_CLR << lobbyID << "..." << std::endl << DEF_CLR;
            }

            Json j;
            j["Type"] = "IsInLobby";
            j["CurrentLobbyID"] = lb->ID;
            sender->Send(j.dump());

            std::cout << INF_CLR << "Lobby confirmation sent to " << HASH_CLR(sender) << std::endl << DEF_CLR;
        }
        SerializeLobbiesToJson(sender);
    }
    else if (receivedData["Type"] == "LeaveLobby")
    {
        for (const auto& lb : m_Lobbies)
        {
            if (receivedData["ID"] != lb->ID) continue;

            const std::string& playerName = m_Players[sender->GetName()];

            lb->RemovePlayerFromLobby(playerName);
            std::cout << STS_CLR << "Player " << HASH_STRING_CLR(playerName) << STS_CLR << " has left lobby: " << INF_CLR << receivedData["ID"] << std::endl << DEF_CLR;

            m_Players.erase(sender->GetName());
            std::cout << STS_CLR << "Unregistered player: " << HASH_STRING_CLR(playerName) << STS_CLR << " from server." << std::endl << DEF_CLR;

            if (m_StartedGames.contains(lb->ID) && lb->IsLobbyEmpty())
            {
                m_StartedGames[lb->ID] = nullptr;
                m_StartedGames.erase(lb->ID);
                std::cout << STS_CLR << "Closing game " << INF_CLR << lb->ID << "..." << std::endl << DEF_CLR;
            }
        }
    }
    else if (receivedData["Type"] == "IsLobbyFull")
    {
        for (const auto& lb : m_Lobbies)
        {
            if (receivedData["ID"] != lb->ID) continue;

            if (lb->IsLobbyFull())
            {
                Lobby* lobby = m_StartedGames[receivedData["ID"]];

                Json j;
                j["Type"] = "SetPlayerShape";
                j["PlayerX"] = lobby->PlayerX;
                j["PlayerO"] = lobby->PlayerO;
                j["Starter"] = rand() % 100 <= 50 ? lobby->PlayerO : lobby->PlayerX;

                int i = 0;
                for (auto [adressIP, player] : m_Players)
                {
                    if (i == 2) break;

                    if (lb->IsInLobby(player))
                    {
                        m_GameServer->GetClientByName(adressIP)->Send(j.dump());
                        i++;
                        std::cout << INF_CLR << "Lobby's full confirmation sent to " << HASH_CLR(sender) << std::endl << DEF_CLR;
                    }
                }
            }
        }
    }
    ////////// Game State //////////
    else if (receivedData["Type"] == "OpponentMove")
    {
        Lobby* lb = m_StartedGames[receivedData["ID"]];
        const std::string& opponentName = lb->GetOpponentName(receivedData["PlayerName"]);

        for (auto [adressIP, player] : m_Players)
        {
            if (player == opponentName)
            {
                m_GameServer->GetClientByName(adressIP)->Send(receivedData.dump());

                std::cout << STS_CLR << "Player " << HASH_STRING_CLR(opponentName) << STS_CLR << " has made a move in lobby: " << INF_CLR << receivedData["ID"] << std::endl << DEF_CLR;

                return;
            }
        }
    }
    else
    {
        std::cout << WRN_CLR << "Received JSON from " << HASH_CLR(sender) << WRN_CLR << " contains an unknown type." << std::endl << DEF_CLR;
    }
}

void ServerApp::CleanUpGameServer()
{
    std::cout << Color::Cyan << "============== Starting Game Server Clean Up ==============" << std::endl;
    try
    {
        for (auto& c : m_GameServer->GetConnections())
        {
            c.Kick();
        }

        int count = 0;
        m_GameServer->CleanClosedConnections([&](ClientPtr c) { ++count; });
        if (count > 0)
            std::cout << INF_CLR << "Closed " << count << " connection" << (count > 1 ? "s" : "") << "." << std::endl;

        for (auto& [id, lobby] : m_StartedGames)
        {
            NULLPTR(lobby);
        }
        if (!m_StartedGames.empty())
            std::cout << INF_CLR << "Ended " << m_StartedGames.size() << " started game" << (m_StartedGames.size() > 1 ? "s" : "") << "." << std::endl;
        m_StartedGames.clear();

        for (auto lb : m_Lobbies)
        {
            RELEASE(lb);
        }
        if (!m_Lobbies.empty())
            std::cout << INF_CLR << "Deleted " << m_Lobbies.size() << " lobb" << (m_Lobbies.size() > 1 ? "ies" : "y") << "." << std::endl;
        m_Lobbies.clear();

        m_GameServer->Close();
        delete m_GameServer;
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << ERR_CLR << "Game server clean up failed: " << e.what() << std::endl << DEF_CLR;
    }
    std::cout << Color::Cyan << "============== Game Server Clean Up Complete ==============" << std::endl << DEF_CLR;
}

#pragma endregion

#pragma region Web Server

bool ServerApp::InitWebServer()
{
    std::cout << WEB_PFX << Color::Cyan << "===== Starting Web Server Initialization  ===========" << std::endl << DEF_CLR;
    try
    {
        m_WebServer = new HtmlServer();
        m_WebServer->Open(DEFAULT_PORT + 1);
        std::cout << WEB_PFX << "Web server is listening on port " << DEFAULT_PORT + 1 << "..." << std::endl;
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << WEB_PFX << WRN_CLR << "Web server is unable to start: " << e.what() << std::endl << DEF_CLR;
        RELEASE(m_WebServer);
        return false;
    }
    std::cout << WEB_PFX << Color::Cyan << "===== Web Server Initialization Complete  ===========" << std::endl << DEF_CLR;
    return true;
}

void ServerApp::HandleWebServer()
{
    try
    {
        m_WebServer->CheckNetwork();

        WebClientPtr newClient;
        while ((newClient = m_WebServer->FindNewClient()) != nullptr)
        {
            std::cout << WEB_PFX << STS_CLR << "New connection from " << HASH_CLR(newClient) << STS_CLR << " has been established." << std::endl << DEF_CLR;
        }

        WebClientPtr sender;
        while ((sender = m_WebServer->FindClientWithPendingData()) != nullptr)
        {
            HandleWebConnection(sender);
        }

        m_WebServer->CleanClosedHtmlConns([](WebClientPtr c)
            {
                std::cout << WEB_PFX << STS_CLR << "Connection from " << HASH_CLR(c) << STS_CLR << " has been closed." << std::endl << DEF_CLR;
            });
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << WEB_PFX << ERR_CLR << "The web server has encountered an error: " << e.what() << std::endl << DEF_CLR;
    }
}

void ServerApp::HandleWebConnection(WebClientPtr sender)
{
    constexpr int maxWouldBlockErrors = 10;
    int wbe = 0;
    std::string data;
    while (true)
    {
        try
        {
            data = sender->Receive();
            break;
        }
        catch (const TcpIp::TcpIpException& e)
        {
            if (e.Context == WSAEWOULDBLOCK && wbe < maxWouldBlockErrors)
            {
                ++wbe; // "Try again" error, ignore
                continue;
            }

            std::cout << WEB_PFX << ERR_CLR << "Error while receiving data from " << HASH_CLR(sender) << DEF_CLR << ": " << e.what() << std::endl;
            return;
        }
    }

    if (data.starts_with("GET "))
    {
        std::string page = data.substr(4, data.find(' ', 4) - 4);
        std::cout << WEB_PFX << HASH_CLR(sender) << DEF_CLR << " sent a GET request for " << page << ". ";

        if (page == "/")
        {
            std::cout << "Sending root page." << std::endl;
            sender->Send("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"

                // TODO: Send the actual page, lobbies with href to /watch/lobbyid

                "<html><body><h1>Hello " + sender->Address + ":" + std::to_string(sender->Port) + "</h1></body></html>");
        }
        else if (page == "/favicon.ico")
        {
            // We don't have a favicon, so just send a 404
            std::cout << "Sending 404." << std::endl;
            sender->Send("HTTP/1.1 404 Not Found\r\n\r\n");
        }

        // TODO: page.starts_with("/watch/") -> send the watch page for the lobby

        /*
            The client should get something like this: (maybe with a link to the root page)

            Lobby ID: 123456
            Player X: "Player 1"
            Player O: "Player 2"
            Turn: X


             X | O | X
            -----------
             X | X | O
            -----------
             O | X | O

        */

        else
        {
            std::cout << "Redirecting to root page." << std::endl;
            sender->Send("HTTP/1.1 301 Moved Permanently\r\nLocation: /\r\n\r\n");
        }
    }
    else
    {
        std::cout << WEB_PFX << HASH_CLR(sender) << DEF_CLR << " sent an unknown request." << std::endl;
    }

    // Close the connection
    sender->Kick();
}

void ServerApp::CleanUpWebServer()
{
    if (!m_WebServer) return;

    std::cout << WEB_PFX << Color::Cyan << "======== Starting Web Server Clean Up  ==============" << std::endl << DEF_CLR;
    try
    {
        for (auto& c : m_WebServer->GetHtmlConns())
        {
            c.Kick();
        }

        int count = 0;
        m_WebServer->CleanClosedHtmlConns([&](WebClientPtr c) { ++count; });

        if (count > 0)
            std::cout << WEB_PFX << "Closed " << count << " connection" << (count > 1 ? "s" : "") << "." << std::endl;

        m_WebServer->Close();
        delete m_WebServer;
    }
    catch (const TcpIp::TcpIpException& e)
    {
        std::cout << WEB_PFX << ERR_CLR << "Web server clean up failed: " << e.what() << std::endl << DEF_CLR;
    }
    std::cout << WEB_PFX << Color::Cyan << "======== Web Server Clean Up Complete  ==============" << std::endl << DEF_CLR;
}

#pragma endregion

#pragma region Lobbying

size_t ServerApp::FindPlayer(const std::string& name) const
{
    for (size_t i = 0; i < m_Lobbies.size(); ++i)
    {
        if (m_Lobbies[i]->IsInLobby(name))
            return i;
    }
    return -1;
}

void ServerApp::CreateLobbies()
{
    for (int i = 0; i < 3; i++)
    {
        m_Lobbies.emplace_back(new Lobby());
    }
}

void ServerApp::SerializeLobbiesToJson(ClientPtr sender) const
{
    Json lobbyListJson;
    lobbyListJson["Type"] = "Lobby";

    for (const auto& lobby : m_Lobbies)
    {
        lobbyListJson["Lobbies"].push_back(lobby->Serialize());
    }

    sender->Send(lobbyListJson.dump());
}

#pragma endregion
