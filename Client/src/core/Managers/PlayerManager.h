#pragma once
#include "src/core/Player.h"
#include "IManager.h"

class PlayerManager : public IManager
{
public:

    PlayerManager();
    ~PlayerManager() override;

    void Init() override;
    void Clear() override;

    void SwitchPlayerTurn();

    void CreateNewPlayer(const std::string& name, const sf::Color color, const PlayerShapeType shapeType);
    void UnregisterPlayer(Player* player);

    const std::vector<Player*>& GetAllPlayers() { return m_RegisteredPlayers; }
    Player* GetPlayer(int index) const { return m_RegisteredPlayers[index]; }
    static Player* GetCurrentPlayer() { return m_CurrentPlayer; }
    static Player* GetOpponentPlayer() { return m_OpponentPlayer; }
    int GetPlayerCount() const { return m_PlayerCount; }
    int GetOpponentPlayerIndex() const{ return m_CurrentPlayerIndex == 0 ? 1 : 0; }

private:

    std::vector<Player*> m_RegisteredPlayers;
    static Player* m_CurrentPlayer;
    static Player* m_OpponentPlayer;

    int m_PlayerCount;
    int m_CurrentPlayerIndex;
};
