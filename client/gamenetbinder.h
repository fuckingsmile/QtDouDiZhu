#pragma once
#include <QObject>
#include "cards.h"

class NetworkManager;

class GameNetBinder : public QObject
{
    Q_OBJECT
public:
    explicit GameNetBinder(QObject *parent = nullptr);

    void connectToServer(const QString &ip, quint16 port);
    void sendCallScore(int score, int playerId);
    void sendPlayCards(int playerId, const Cards &cards);
    void sendPass(int playerId);
    void sendRequestRestart(int playerId);

signals:
    void connected();
    void disconnected();

    void welcomeReceived(int playerId);
    void roomInfoReceived(int count);
    void startGameReceived();

    void dealCardsReceived(Cards cards, Cards bottomCards);
    void callTurnReceived(int playerId);
    void playerCalledReceived(int playerId, int score);
    void lordConfirmedReceived(int lordPlayerId, Cards bottomCards);
    void playTurnReceived(int playerId);
    void playerPlayedReceived(int playerId, Cards cards);
    void playerPassedReceived(int playerId);
    void gameOverReceived(int winnerId,
                          int lordPlayerId,
                          bool lordWin,
                          int score1,
                          int score2,
                          int score3);;

private:
    NetworkManager *m_net;
};
