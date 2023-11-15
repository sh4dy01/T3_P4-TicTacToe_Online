#pragma once
#include "TicTacToe.h"
#include "GraphicBoard.h"
#include <SFML/System/Time.hpp>

class Window;
class sf::Shape;

class ClientApp
{
    // Private constructor for singleton
    ClientApp() = default;
    ClientApp(const ClientApp&) = delete;
    ~ClientApp() = default;

public:
    /// <summary>
    /// Get the singleton instance of the ClientApp. The instance is created on the first call to this method.
    /// </summary>
    static ClientApp& GetInstance() { static ClientApp instance; return instance; }

    void DrawBoard();
    /// <summary>
    /// Initialize the ClientApp. This method must be called before calling Run().
    /// </summary>
    void Init();
    /// <summary>
    /// Run the ClientApp. This method will not return until the ClientApp is shut down.
    /// </summary>
    void Run();
    /// <summary>
    /// Shutdown the ClientApp. This method will cause Run() to return.
    /// </summary>
    void Shutdown() { m_IsRunning = false; }

private: // Methods
    /// Update the ClientApp. Called once per frame.
    void Update(sf::Time delta);
    void SwitchPlayerTurn();
    void PlacePlayerPieceOnBoard(unsigned int i);

    void ClearBoard();
    void CheckIfMouseHoverBoard();
    bool IsMouseHoverPiece(unsigned int i);

    /// Perform any cleanup tasks (e.g. delete pointers). Called before Run() returns.
    void Cleanup();

private: // Fields
    bool m_IsRunning = false;

    Window* m_Window = nullptr;

    GraphicBoard m_Board;
    Player m_PlayerOne;
    Player m_PlayerTwo;

    sf::Time m_PlayerTurnTimer = sf::seconds(0);

    std::vector<sf::Drawable*> m_GamePieces;

    bool m_IsPlayerOneTurn = true;

};
