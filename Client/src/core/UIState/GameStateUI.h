#pragma once
#include "src/core/Components/ProgressBar.h"
#include "src/core/Managers/UIStateManager.h"

struct PlayerData;
class Player;

class GameStateUI : public UIStateManager
{
public:

    GameStateUI(Window* window);
    ~GameStateUI() override;

    void Init() override;
    void Clear() override;

    void InitPlayerScores(const std::map<TicTacToe::Piece, Player>& allPlayers);
    void InitProgressBar(const sf::Color& color, const float maxValue);

    void UpdatePlayerTurnText(const PlayerData* data);
    void UpdateGameStateText(const std::string& text, sf::Color color = sf::Color::White);
    void UpdatePlayerScore(const TicTacToe::Piece, const std::string&, unsigned int score);
    void UpdateProgressBar(float value) const { m_ProgressBar->SetValue(value); }
    void SetGameModeAndIDText(unsigned int& id, const std::string& gameMode);
    void SetUserName(const std::string& username);

private:

    sf::Text* m_GameStateText;
    sf::Text* m_GameModeText;
    sf::Text* m_LobbyIDText;
    sf::Text* m_UserName;
    sf::Text* m_Title;
    ProgressBar* m_ProgressBar;

    std::unordered_map<TicTacToe::Piece, sf::Text*> m_PlayerScoreTexts;
};