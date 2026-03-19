#include "networkmanager.h"
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include "jsonhelper.h"
NetworkManager::NetworkManager(QObject *parent)
    : QObject(parent)
{
    m_socket = new QTcpSocket(this);

    connect(m_socket, &QTcpSocket::connected,
            this, &NetworkManager::connected);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &NetworkManager::disconnected);
    connect(m_socket, &QTcpSocket::readyRead,
            this, &NetworkManager::onReadyRead);
}

void NetworkManager::connectToServer(const QString &ip, quint16 port)
{
    m_socket->connectToHost(ip, port);
}

void NetworkManager::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
}

void NetworkManager::sendCallScore(int score, int playerId)
{
       QJsonObject obj;
       obj["type"] = "call_score";
       obj["playerId"] = playerId;
       obj["score"] = score;
       sendJson(obj);
}

void NetworkManager::sendPlayCards(int playerId, Cards cards)
{
        QJsonObject obj;
        obj["type"] = "play_cards";
        obj["playerId"] = playerId;
        obj["cards"] = cardsToJson(cards);
        sendJson(obj);
}

void NetworkManager::sendPass(int playerId)
{
        QJsonObject obj;
        obj["type"] = "pass";
        obj["playerId"] = playerId;
        sendJson(obj);
}

void NetworkManager::sendRequestRestart(int playerId)
{
        QJsonObject obj;
        obj["type"] = "request_restart";
        obj["playerId"] = playerId;
        sendJson(obj);
}

void NetworkManager::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "client invalid json:" << line;
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "welcome") {
            emit welcomeReceived(obj["playerId"].toInt());
        }
        else if (type == "room_info") {
            emit roomInfoReceived(obj["count"].toInt());
        }
        else if (type == "start_game") {
            emit startGameReceived();
        }

        else if (type == "deal_cards") {
            QJsonArray arr = obj["cards"].toArray();
            Cards cards = jsonToCards(arr);
            QJsonArray bottomArr = obj["bottomCards"].toArray();
            Cards bottomCards = jsonToCards(bottomArr);
            emit dealCardsReceived(cards, bottomCards);
        }

        else if (type == "call_turn") {
            emit callTurnReceived(obj["playerId"].toInt());
        }
        else if (type == "player_called") {
            emit playerCalledReceived(obj["playerId"].toInt(),
                                      obj["score"].toInt());
        }
        else if (type == "lord_confirmed") {
            int lordPlayerId = obj["lordPlayerId"].toInt();
            QJsonArray arr = obj["bottomCards"].toArray();
            Cards bottomCards = jsonToCards(arr);
            emit lordConfirmedReceived(lordPlayerId, bottomCards);
        }

        else if (type == "play_turn") {
            emit playTurnReceived(obj["playerId"].toInt());
        }
        else if (type == "player_played") {
            int playerId = obj["playerId"].toInt();
            QJsonArray arr = obj["cards"].toArray();
            Cards cards = jsonToCards(arr);
            emit playerPlayedReceived(playerId, cards);
        }
        else if (type == "player_passed") {
            emit playerPassedReceived(obj["playerId"].toInt());
        }

        else if (type == "game_over") {
            emit gameOverReceived(
                obj["winnerId"].toInt(),
                obj["lordPlayerId"].toInt(),
                obj["lordWin"].toBool(),
                obj["score1"].toInt(),
                obj["score2"].toInt(),
                obj["score3"].toInt()
            );
        }

        qDebug() << "client recv:" << obj;
    }
}
