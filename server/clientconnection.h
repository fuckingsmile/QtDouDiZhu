#ifndef CLIENTCONNECTION_H
#define CLIENTCONNECTION_H

#include <QTcpSocket>
#include <QJsonObject>
#include <QObject>

class ClientConnection : public QObject
{
    Q_OBJECT
public:
    explicit ClientConnection(qintptr socketDescriptor, QObject *parent = nullptr);

    QTcpSocket* socket() const;
    void sendJson(const QJsonObject& obj);

    int playerId() const;
    void setPlayerId(int id);

signals:
    void jsonReceived(ClientConnection* client, QJsonObject obj);
    void disconnected(ClientConnection* client);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket* m_socket;
    int m_playerId;
};

#endif // CLIENTCONNECTION_H
