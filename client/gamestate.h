#ifndef GAMESTATE_H
#define GAMESTATE_H

#include <QObject>
#include "cards.h"

class GameState : public QObject
{
    Q_OBJECT
public:
    explicit GameState(QObject *parent = nullptr);

    int playerId() const;
    void setPlayerId(int id);

    Cards netCards() const;
    void setNetCards(const Cards &cards);

    int lastPlayPlayerId() const;
    void setLastPlayPlayerId(int id);

    int currentCallScore() const;
    void setCurrentCallScore(int score);
private:
    int m_playerId = -1;
    Cards m_netCards;
    int m_lastPlayPlayerId = -1;
    int m_currentCallScore = 0;
};

#endif // GAMESTATE_H
