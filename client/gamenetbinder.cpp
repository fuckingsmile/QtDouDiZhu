#include "gamenetbinder.h"
#include "networkmanager.h"

GameNetBinder::GameNetBinder(QObject *parent)
    : QObject(parent)
{
    m_net = new NetworkManager(this);

    connect(m_net, &NetworkManager::connected,
            this, &GameNetBinder::connected);
    connect(m_net, &NetworkManager::disconnected,
            this, &GameNetBinder::disconnected);

    connect(m_net, &NetworkManager::welcomeReceived,
            this, &GameNetBinder::welcomeReceived);
    connect(m_net, &NetworkManager::roomInfoReceived,
            this, &GameNetBinder::roomInfoReceived);
    connect(m_net, &NetworkManager::startGameReceived,
            this, &GameNetBinder::startGameReceived);

    connect(m_net, &NetworkManager::dealCardsReceived,
            this, &GameNetBinder::dealCardsReceived);
    connect(m_net, &NetworkManager::callTurnReceived,
            this, &GameNetBinder::callTurnReceived);
    connect(m_net, &NetworkManager::playerCalledReceived,
            this, &GameNetBinder::playerCalledReceived);
    connect(m_net, &NetworkManager::lordConfirmedReceived,
            this, &GameNetBinder::lordConfirmedReceived);
    connect(m_net, &NetworkManager::playTurnReceived,
            this, &GameNetBinder::playTurnReceived);
    connect(m_net, &NetworkManager::playerPlayedReceived,
            this, &GameNetBinder::playerPlayedReceived);
    connect(m_net, &NetworkManager::playerPassedReceived,
            this, &GameNetBinder::playerPassedReceived);
    connect(m_net, &NetworkManager::gameOverReceived,
            this, &GameNetBinder::gameOverReceived);
}

void GameNetBinder::connectToServer(const QString &ip, quint16 port)
{
    m_net->connectToServer(ip, port);
}

void GameNetBinder::sendCallScore(int score, int playerId)
{
    m_net->sendCallScore(score, playerId);
}

void GameNetBinder::sendPlayCards(int playerId, const Cards &cards)
{
    m_net->sendPlayCards(playerId, cards);
}

void GameNetBinder::sendPass(int playerId)
{
    m_net->sendPass(playerId);
}

void GameNetBinder::sendRequestRestart(int playerId)
{
    m_net->sendRequestRestart(playerId);
}
