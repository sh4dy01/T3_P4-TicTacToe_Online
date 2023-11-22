#pragma once
#include <string>
#include "Managers/Resources/PlayerShapeRegistry.h"

struct PlayerData
{
    std::string Name;
    TicTacToe::PieceID Id;
    sf::Color Color;
    PlayerShapeType ShapeType;
};

class Player
{
public:

    Player();
    Player(const std::string& name, const sf::Color color, const PlayerShapeType);
    ~Player();

    void SetColor(const sf::Color color) { m_Data.Color = color; }

    sf::Color GetColor() const { return m_Data.Color; }
    TicTacToe::PieceID GetPlayerID() const { return m_Data.Id; }
    std::string GetName() const { return m_Data.Name; }
    PlayerShapeType GetShapeType() const { return m_Data.ShapeType; }
    PlayerData* GetData() { return &m_Data; }

    void SetName(const std::string& name) { m_Data.Name = name; }

private:

    PlayerData m_Data;

};
