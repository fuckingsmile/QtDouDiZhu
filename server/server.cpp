#include "server.h"
#include <QDebug>

Server::Server(QObject *parent)
    : QTcpServer(parent)
{
    m_room = new Room(this);
}

void Server::startServer(quint16 port)
{
    if (listen(QHostAddress::Any, port)) {
        qDebug() << "Server started, port =" << port;
    } else {
        qDebug() << "Server start failed:" << errorString();
    }
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    qDebug() << "New client connected:" << socketDescriptor;

    ClientConnection* client = new ClientConnection(socketDescriptor, this);
    m_room->addPlayer(client);
}
