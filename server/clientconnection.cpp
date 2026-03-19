#include "clientconnection.h"
#include <QJsonDocument>
#include <QDebug>

ClientConnection::ClientConnection(qintptr socketDescriptor, QObject *parent)
    : QObject(parent), m_playerId(-1)
{
    m_socket = new QTcpSocket(this);
    m_socket->setSocketDescriptor(socketDescriptor);

    connect(m_socket, &QTcpSocket::readyRead,
            this, &ClientConnection::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected,
            this, &ClientConnection::onDisconnected);
}

QTcpSocket* ClientConnection::socket() const
{
    return m_socket;
}

void ClientConnection::sendJson(const QJsonObject &obj)
{
    QJsonDocument doc(obj);
    QByteArray data = doc.toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
}

int ClientConnection::playerId() const
{
    return m_playerId;
}

void ClientConnection::setPlayerId(int id)
{
    m_playerId = id;
}

void ClientConnection::onReadyRead()
{
    while (m_socket->canReadLine()) {
        QByteArray line = m_socket->readLine().trimmed();
        if (line.isEmpty()) {
            continue;
        }
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(line, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            qDebug() << "Invalid json:" << line;
            continue;
        }
        emit jsonReceived(this, doc.object());
    }
}

void ClientConnection::onDisconnected()
{
    emit disconnected(this);
}
