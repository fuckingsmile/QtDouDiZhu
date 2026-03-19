#ifndef NETWORKMANAGER_H
#define NETWORKMANAGER_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include "cards.h"

class NetworkManager : public QObject
{
    Q_OBJECT
public:
    explicit NetworkManager(QObject *parent = nullptr);

    void connectToServer(const QString& ip, quint16 port);
    void sendJson(const QJsonObject& obj);

    void sendCallScore(int score, int playerId);

    void sendPlayCards(int playerId, Cards cards);
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

private slots:
    void onReadyRead();

private:
    QTcpSocket* m_socket;
};

#endif // NETWORKMANAGER_H
