#include "gamestate.h"

GameState::GameState(QObject *parent) : QObject(parent)
{
}

int GameState::playerId() const
{
    return m_playerId;
}

void GameState::setPlayerId(int id)
{
    m_playerId = id;
}

Cards GameState::netCards() const
{
    return m_netCards;
}

void GameState::setNetCards(const Cards &cards)
{
    m_netCards = cards;
}

int GameState::lastPlayPlayerId() const
{
    return m_lastPlayPlayerId;
}

void GameState::setLastPlayPlayerId(int id)
{
    m_lastPlayPlayerId = id;
}

int GameState::currentCallScore() const
{
    return m_currentCallScore;
}

void GameState::setCurrentCallScore(int score)
{
    m_currentCallScore = score;
}
