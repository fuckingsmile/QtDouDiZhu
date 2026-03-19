#include "room.h"
#include <QDebug>
#include <QJsonArray>
#include "jsonhelper.h"
#include "playhand.h"
Room::Room(QObject *parent)
    : QObject(parent)
{
        m_currentCallPlayer = 1;
        m_callCount = 0;
        m_maxScore = 0;
        m_lordPlayerId = -1;
        m_passCount = 0;
}

void Room::addPlayer(ClientConnection *client)
{
    if (m_players.size() >= 3) {
        QJsonObject obj;
        obj["type"] = "room_full";
        client->sendJson(obj);
        client->socket()->disconnectFromHost();
        client->deleteLater();
        return;
    }
    int id = m_players.size() + 1;
    client->setPlayerId(id);
    m_players.append(client);
    connect(client, &ClientConnection::jsonReceived,
            this, &Room::onJsonReceived);
    connect(client, &ClientConnection::disconnected,
            this, &Room::onClientDisconnected);
    QJsonObject welcome;
    welcome["type"] = "welcome";
    welcome["playerId"] = id;
    client->sendJson(welcome);
    sendRoomInfo();
    startGameIfReady();
    qDebug() << "Player joined, id =" << id;
}

void Room::startCallLord()
{
       m_currentCallPlayer = 1;
       m_callCount = 0;
       m_maxScore = 0;
       m_lordPlayerId = -1;
       qDebug() << "Call lord start";
       notifyTurnToCall();
}

void Room::notifyTurnToCall()
{
        QJsonObject obj;
        obj["type"] = "call_turn";
        obj["playerId"] = m_currentCallPlayer;
        broadcast(obj);
        qDebug() << "Now player" << m_currentCallPlayer << "call score";
}

void Room::startPlayHand()
{
    m_currentPlayPlayer = m_lordPlayerId;
        m_lastPlayPlayer = -1;
        m_lastPlayCards.clear();
        m_passCount = 0;
        qDebug() << "Play hand start, first player =" << m_currentPlayPlayer;
        notifyTurnToPlay();
}

void Room::notifyTurnToPlay()
{
    QJsonObject obj;
       obj["type"] = "play_turn";
       obj["playerId"] = m_currentPlayPlayer;
       broadcast(obj);
       qDebug() << "Now player" << m_currentPlayPlayer << "play hand";
}

void Room::broadcast(const QJsonObject &obj)
{
    for (ClientConnection* client : m_players) {
        client->sendJson(obj);
    }
}

void Room::sendRoomInfo()
{
    QJsonObject obj;
    obj["type"] = "room_info";
    obj["count"] = m_players.size();

    QJsonArray arr;
    for (ClientConnection* client : m_players) {
        arr.append(client->playerId());
    }
    obj["players"] = arr;
    broadcast(obj);
}

void Room::startGameIfReady()
{
    if (m_players.size() != 3) {
            return;
        }
        QJsonObject obj;
        obj["type"] = "start_game";
        broadcast(obj);
        qDebug() << "3 players ready, game start";
        dealCards();
        startCallLord();
}

void Room::initAllCards()
{
    m_allCards.clear();
    m_last3Cards.clear();
    for (int p = Card::Card_Begin + 1; p < Card::Card_SJ; ++p)
    {
        for (int s = Card::Suit_Begin + 1; s < Card::Suit_End; ++s)
        {
            Card card(static_cast<Card::CardPoint>(p),
                      static_cast<Card::CardSuit>(s));
            m_allCards.add(card);
        }
    }
    m_allCards.add(Card(Card::Card_SJ, Card::Suit_Begin));
    m_allCards.add(Card(Card::Card_BJ, Card::Suit_Begin));
}

Card Room::takeOneCard()
{
    Card card = m_allCards.takeRandomCard();
    return card;
}

void Room::dealCards()
{
    m_playerCards.clear();

    initAllCards();
        QList<Cards> playerCards;
        playerCards << Cards() << Cards() << Cards();
        for (int i = 0; i < 17; ++i) {
            for (int j = 0; j < 3; ++j) {
                Card c = takeOneCard();
                playerCards[j].add(c);
            }
        }

        for (int i = 0; i < 3; ++i) {
            Card c = takeOneCard();
            m_last3Cards.add(c);
        }

        //登记 分发
        for (int i = 0; i < m_players.size(); ++i) {
            int playerId = m_players[i]->playerId();

                // 服务器保存这名玩家的手牌
                m_playerCards[playerId] = playerCards[i];

                qDebug() << "server player" << playerId
                         << "real count =" << playerCards[i].cardCount();
                playerCards[i].printAllCardInfo();

                QJsonObject obj;
                obj["type"] = "deal_cards";
                obj["cards"] = cardsToJson(playerCards[i]);
                obj["count"] = playerCards[i].cardCount();
                obj["bottomCards"] = cardsToJson(m_last3Cards);
                m_players[i]->sendJson(obj);
        }

        qDebug() << "Cards dealt. Bottom cards =" << m_last3Cards.cardCount();
}

void Room::resetGameState()
{
    m_restartReadyPlayers.clear();

        m_playerCards.clear();
        m_allCards.clear();
        m_last3Cards.clear();

        m_currentCallPlayer = 1;
        m_callCount = 0;
        m_maxScore = 0;
        m_lordPlayerId = -1;

        m_currentPlayPlayer = -1;
        m_lastPlayPlayer = -1;
        m_lastPlayCards.clear();
        m_passCount = 0;
}

void Room::onJsonReceived(ClientConnection *client, QJsonObject obj)
{
    QString type = obj["type"].toString();

    if (type == "call_score") {
        int playerId = obj["playerId"].toInt();
        int score = obj["score"].toInt();

        qDebug() << "player" << playerId << "call score =" << score;

        // 广播给所有客户端：谁叫了多少分
        QJsonObject ret;
        ret["type"] = "player_called";
        ret["playerId"] = playerId;
        ret["score"] = score;
        broadcast(ret);

        // 更新最高分
        if (score > m_maxScore) {
            m_maxScore = score;
            m_lordPlayerId = playerId;
        }

        m_callCount++;

        // 3 分直接确定地主
        if (score == 3) {
            m_playerCards[playerId].add(m_last3Cards);
            QJsonObject lordObj;
            lordObj["type"] = "lord_confirmed";
            lordObj["lordPlayerId"] = playerId;
            lordObj["bottomCards"] = cardsToJson(m_last3Cards);
            broadcast(lordObj);
            qDebug() << "Lord confirmed =" << playerId;
            startPlayHand();
            return;
        }
        // 三个人都叫完
        if (m_callCount >= 3) {
            if (m_lordPlayerId == -1) {
                m_lordPlayerId = 1;
            }

             m_playerCards[m_lordPlayerId].add(m_last3Cards);

            QJsonObject lordObj;
            lordObj["type"] = "lord_confirmed";
            lordObj["lordPlayerId"] = m_lordPlayerId;
            lordObj["bottomCards"] = cardsToJson(m_last3Cards);
            broadcast(lordObj);

            qDebug() << "Lord confirmed =" << m_lordPlayerId;
            startPlayHand();
            return;
        }
        // 轮到下一个玩家
        m_currentCallPlayer++;
        if (m_currentCallPlayer > 3) {
            m_currentCallPlayer = 1;
        }
        notifyTurnToCall();
        return;
    }

    if (type == "play_cards") {
        int playerId = obj["playerId"].toInt();
        QJsonArray arr = obj["cards"].toArray();
        Cards cards = jsonToCards(arr);

        qDebug() << "[play_cards]"
                 << "playerId =" << playerId
                 << ", currentPlayPlayer =" << m_currentPlayPlayer
                 << ", lastPlayPlayer =" << m_lastPlayPlayer
                 << ", lastPlayCardsCount =" << m_lastPlayCards.cardCount()
                 << ", passCount =" << m_passCount
                 << ", tryPlayCount =" << cards.cardCount();
        // 1. 不是当前玩家，直接拒绝
        if (playerId != m_currentPlayPlayer) {
            qDebug() << "reject: not current player, current =" << m_currentPlayPlayer;
            return;
        }

        // 2. 玩家手里没有这些牌，直接拒绝
        if (!m_playerCards[playerId].contains(cards)) {
            qDebug() << "reject: player" << playerId << "does not own these cards";
            return;
        }

        // 3. 牌型是否合法
        PlayHand hand(cards);
        if (hand.getHandType() == PlayHand::Hand_Unknown) {
            qDebug() << "reject: invalid hand type";
            return;
        }

        // 4. 不是新一轮首出时，检查能不能压过上一手
        if (m_lastPlayPlayer != -1 && m_lastPlayPlayer != playerId) {
            PlayHand lastHand(m_lastPlayCards);
            if (!hand.canBeat(lastHand)) {
                qDebug() << "reject: cannot beat last hand";
                return;
            }
        }

        // 5. 出牌成功，服务器删手牌
        m_playerCards[playerId].remove(cards);

        qDebug() << "player" << playerId
                 << "played ok, remain =" << m_playerCards[playerId].cardCount();

        // 判断是否出完牌
        if (m_playerCards[playerId].cardCount() == 0) {
            qDebug() << "Game over, winner =" << playerId;

            bool lordWin = (playerId == m_lordPlayerId);

            // 先按最基础规则算
            // 地主赢：地主 +3，两个农民各 -3
            // 农民赢：地主 -3，两个农民各 +3
            int score1 = 0;
            int score2 = 0;
            int score3 = 0;

            auto calcScore = [&](int pid) -> int {
                if (pid == m_lordPlayerId) {
                    return lordWin ? 3 : -3;
                } else {
                    return lordWin ? -3 : 3;
                }
            };

            score1 = calcScore(1);
            score2 = calcScore(2);
            score3 = calcScore(3);

            QJsonObject over;
            over["type"] = "game_over";
            over["winnerId"] = playerId;
            over["lordPlayerId"] = m_lordPlayerId;
            over["lordWin"] = lordWin;
            over["score1"] = score1;
            over["score2"] = score2;
            over["score3"] = score3;

            broadcast(over);
            return;
        }

        // 6. 更新桌面最后一手
        m_lastPlayPlayer = playerId;
        m_lastPlayCards = cards;
        m_passCount = 0;

        // 7. 广播出牌消息
        QJsonObject ret;
        ret["type"] = "player_played";
        ret["playerId"] = playerId;
        ret["cards"] = cardsToJson(cards);
        broadcast(ret);

        // 8. 切到下一个玩家
        m_currentPlayPlayer++;
        if (m_currentPlayPlayer > 3) {
            m_currentPlayPlayer = 1;
        }

        notifyTurnToPlay();
        return;
    }
    if (type == "pass") {
        int playerId = obj["playerId"].toInt();

        // 1. 不是当前玩家，拒绝
        if (playerId != m_currentPlayPlayer) {
            qDebug() << "reject pass: not current player, current =" << m_currentPlayPlayer;
            return;
        }

        // 2. 如果当前就是最后出牌的人，不允许 pass
        if (m_lastPlayPlayer == -1 || m_lastPlayPlayer == playerId) {
            qDebug() << "reject pass: player" << playerId << "must play";
            return;
        }

        qDebug() << "player" << playerId << "passed";

        QJsonObject ret;
        ret["type"] = "player_passed";
        ret["playerId"] = playerId;
        broadcast(ret);

        m_passCount++;

        // 3. 如果已经连续两个玩家 pass，这一轮结束
        if (m_passCount >= 2) {

            qDebug() << "[round reset before]"
                     << "lastPlayPlayer =" << m_lastPlayPlayer
                     << ", lastPlayCardsCount =" << m_lastPlayCards.cardCount()
                     << ", currentPlayPlayer =" << m_currentPlayPlayer
                     << ", passCount =" << m_passCount;

            qDebug() << "round reset, last player =" << m_lastPlayPlayer;

            // 清空桌面上一手，表示下一次是新的首出
            m_lastPlayCards.clear();

            // 先记住最后出牌的人
            int lastPlayer = m_lastPlayPlayer;

            // 关键：把“最后出牌的人”也重置掉
            m_lastPlayPlayer = -1;

            // 重新轮到最后出牌的人先出
            m_currentPlayPlayer = lastPlayer;

            // 清空 pass 计数
            m_passCount = 0;

            qDebug() << "[round reset after]"
                     << "lastPlayPlayer =" << m_lastPlayPlayer
                     << ", lastPlayCardsCount =" << m_lastPlayCards.cardCount()
                     << ", currentPlayPlayer =" << m_currentPlayPlayer
                     << ", passCount =" << m_passCount;

            notifyTurnToPlay();
            return;
        }

        // 4. 否则轮到下一个玩家
        m_currentPlayPlayer++;
        if (m_currentPlayPlayer > 3) {
            m_currentPlayPlayer = 1;
        }

        notifyTurnToPlay();
        return;
    }

    if (type == "request_restart") {
        int playerId = obj["playerId"].toInt();

        qDebug() << "player" << playerId << "request restart";

        m_restartReadyPlayers.insert(playerId);

        // 广播一下谁已经点了继续（可选，但建议保留，方便调试）
        QJsonObject readyObj;
        readyObj["type"] = "restart_ready";
        readyObj["playerId"] = playerId;
        readyObj["count"] = m_restartReadyPlayers.size();
        broadcast(readyObj);

        // 三个人都同意后，重新开始
        if (m_restartReadyPlayers.size() == 3) {
            qDebug() << "all players ready, restart game";

            resetGameState();

            QJsonObject obj;
            obj["type"] = "start_game";
            broadcast(obj);

            dealCards();
            startCallLord();
        }

        return;
    }

    qDebug() << "recv from player" << client->playerId() << obj;
}
void Room::onClientDisconnected(ClientConnection *client)
{
    qDebug() << "player disconnected =" << client->playerId();
    m_players.removeAll(client);
    sendRoomInfo();
    client->deleteLater();
}
