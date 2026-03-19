#pragma once
#include <QObject>
#include "cards.h"

class GameNetBinder;
class GameState;
class GameControl;
class GamePanel;
class Player;

class GameFlowController : public QObject
{
    Q_OBJECT
public:
    explicit GameFlowController(GameNetBinder *net,
                                GameState *state,
                                GameControl *gameCtl,
                                GamePanel *panel,
                                QObject *parent = nullptr);

private slots:
    void onConnected();
    void onRoomInfoReceived(int count);
    void onStartGame();
    void onWelcome(int playerId);
    void onDealCards(Cards cards, Cards bottomCards);
    void onCallTurn(int playerId);
    void onPlayerCalled(int playerId, int score);
    void onLordConfirmed(int lordPlayerId, Cards bottomCards);
    void onPlayTurn(int playerId);
    void onPlayerPlayed(int playerId, Cards cards);
    void onPlayerPassed(int playerId);
    void onGameOver(int winnerId,
                    int lordPlayerId,
                    bool lordWin,
                    int score1,
                    int score2,
                    int score3);

    void onPlayRequested(const Cards &cards);
    void onPassRequested();
    void onCallScoreRequested(int score);
    void onRestartRequested();

private:
    GameNetBinder *m_net;
    GameState *m_state;
    GameControl *m_gameCtl;
    GamePanel *m_panel;
     Player* resolvePlayer(int playerId) const;
};
