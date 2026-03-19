#ifndef ROOM_H
#define ROOM_H

#include <QObject>
#include <QList>
#include <QJsonObject>
#include "clientconnection.h"
#include "cards.h"
#include <QSet>
class Room : public QObject
{
    Q_OBJECT
public:
    explicit Room(QObject *parent = nullptr);

    void addPlayer(ClientConnection* client);//进房函数

    void startCallLord();
    void notifyTurnToCall();

    void startPlayHand();
    void notifyTurnToPlay();

private slots:
    void onJsonReceived(ClientConnection* client, QJsonObject obj);
    void onClientDisconnected(ClientConnection* client);

private:
    void broadcast(const QJsonObject& obj);
    void sendRoomInfo();
    void startGameIfReady();

    void initAllCards();
    Card takeOneCard();
    void dealCards();

private:
    QList<ClientConnection*> m_players;//房间里的所有客户端连接
    Cards m_allCards;//还没发出去的牌
    Cards m_last3Cards;

    int m_currentCallPlayer;//当前轮到谁叫分
    int m_callCount;//已经有多少次叫分发生了
    int m_maxScore;//叫地主时最高分
    int m_lordPlayerId;//地主是谁

    int m_currentPlayPlayer;//当前轮到谁出牌
    int m_lastPlayPlayer;//上一手真正出牌的人是谁
    Cards m_lastPlayCards;//桌面上当前那一手牌

    QMap<int, Cards> m_playerCards;   // key玩家ID value玩家当前真实手牌
    int m_passCount;                  // 连续 pass 次数

    QSet<int> m_restartReadyPlayers;   // 已点击继续游戏的玩家
    void resetGameState();             // 重置一局状态
};

#endif // ROOM_H
