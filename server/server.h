#ifndef SERVER_H
#define SERVER_H

#include <QTcpServer>
#include <QTcpSocket>
#include <QObject>
#include "clientconnection.h"
#include "room.h"

class Server : public QTcpServer
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);

    void startServer(quint16 port);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    Room* m_room;
};

#endif // SERVER_H
