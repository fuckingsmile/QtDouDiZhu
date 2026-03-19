#include "gameflowcontroller.h"
#include "gamenetbinder.h"
#include "gamestate.h"
#include "gamecontrol.h"
#include "gamepanel.h"
#include "player.h"

GameFlowController::GameFlowController(GameNetBinder *net,
                                       GameState *state,
                                       GameControl *gameCtl,
                                       GamePanel *panel,
                                       QObject *parent)
    : QObject(parent),
      m_net(net),
      m_state(state),
      m_gameCtl(gameCtl),
      m_panel(panel)
{
    connect(m_net, &GameNetBinder::connected,
            this, &GameFlowController::onConnected);

    connect(m_net, &GameNetBinder::roomInfoReceived,
            this, &GameFlowController::onRoomInfoReceived);

    connect(m_net, &GameNetBinder::startGameReceived,
            this, &GameFlowController::onStartGame);

    connect(m_net, &GameNetBinder::welcomeReceived,
            this, &GameFlowController::onWelcome);

    connect(m_net, &GameNetBinder::dealCardsReceived,
            this, &GameFlowController::onDealCards);

    connect(m_net, &GameNetBinder::callTurnReceived,
            this, &GameFlowController::onCallTurn);

    connect(m_net, &GameNetBinder::playerCalledReceived,
            this, &GameFlowController::onPlayerCalled);

    connect(m_net, &GameNetBinder::lordConfirmedReceived,
            this, &GameFlowController::onLordConfirmed);

    connect(m_net, &GameNetBinder::playTurnReceived,
            this, &GameFlowController::onPlayTurn);

    connect(m_net, &GameNetBinder::playerPlayedReceived,
            this, &GameFlowController::onPlayerPlayed);

    connect(m_net, &GameNetBinder::playerPassedReceived,
            this, &GameFlowController::onPlayerPassed);

    connect(m_net, &GameNetBinder::gameOverReceived,
            this, &GameFlowController::onGameOver);

    connect(m_panel, &GamePanel::playRequested,
            this, &GameFlowController::onPlayRequested);

    connect(m_panel, &GamePanel::passRequested,
            this, &GameFlowController::onPassRequested);

    connect(m_panel, &GamePanel::callScoreRequested,
            this, &GameFlowController::onCallScoreRequested);

    connect(m_panel, &GamePanel::restartRequested,
            this, &GameFlowController::onRestartRequested);
}

void GameFlowController::onConnected()
{
    m_panel->handleConnected();
}

void GameFlowController::onRoomInfoReceived(int count)
{
    m_panel->handleRoomInfo(count);
}

void GameFlowController::onStartGame()
{
    m_state->setLastPlayPlayerId(-1);
    m_state->setCurrentCallScore(0);

    Player* p1 = resolvePlayer(1);
    Player* p2 = resolvePlayer(2);
    Player* p3 = resolvePlayer(3);

    if (p1) {
        p1->setWin(false);
        p1->setRole(Player::Farmer);
    }
    if (p2) {
        p2->setWin(false);
        p2->setRole(Player::Farmer);
    }
    if (p3) {
        p3->setWin(false);
        p3->setRole(Player::Farmer);
    }

    m_panel->handleStartGame();
    // 网络版：初始化剩余牌数（默认 17 张，后续按广播递减/地主+3 调整）
    if (p1) m_panel->setRemainCount(p1, 17);
    if (p2) m_panel->setRemainCount(p2, 17);
    if (p3) m_panel->setRemainCount(p3, 17);
}

void GameFlowController::onWelcome(int playerId)
{
    m_state->setPlayerId(playerId);
    m_panel->handleWelcome(playerId);
}

void GameFlowController::onDealCards(Cards cards, Cards bottomCards)
{
    m_state->setNetCards(cards);
    m_panel->handleDealCards(cards, bottomCards);
}

void GameFlowController::onCallTurn(int playerId)
{
    m_panel->handleCallTurn(playerId);
}

void GameFlowController::onPlayerCalled(int playerId, int score)
{
    int oldScore = m_state->currentCallScore();

    Player* player = resolvePlayer(playerId);
    m_panel->handlePlayerCalled(player, playerId, score);

    if (score > oldScore) {
        m_state->setCurrentCallScore(score);
    }
}

void GameFlowController::onLordConfirmed(int lordPlayerId, Cards bottomCards)
{
    Player* p1 = resolvePlayer(1);
    Player* p2 = resolvePlayer(2);
    Player* p3 = resolvePlayer(3);

    if (p1) p1->setRole(Player::Farmer);
    if (p2) p2->setRole(Player::Farmer);
    if (p3) p3->setRole(Player::Farmer);

    Player *lordPlayer = resolvePlayer(lordPlayerId);
    if (lordPlayer == nullptr) {
        return;
    }

    m_state->setCurrentCallScore(0);
    lordPlayer->setRole(Player::Lord);

    if (lordPlayerId == m_state->playerId()) {
        Cards cards = lordPlayer->getCards();
        cards.add(bottomCards);
        lordPlayer->setCards(cards);
    }
    else
    {
        // 地主是别人：其剩余牌数 +3（用于渲染牌背）
        if (lordPlayer != nullptr)
        {
            int cur = m_panel->remainCount(lordPlayer);
            if (cur <= 0) cur = 17; // 兜底
            m_panel->setRemainCount(lordPlayer, cur + bottomCards.cardCount());
        }
    }

    m_panel->handleLordConfirmed(lordPlayer, lordPlayerId, bottomCards);
}

void GameFlowController::onPlayTurn(int playerId)
{
    qDebug() << "[client onPlayTurn]"
             << "playerId =" << playerId
             << ", myId =" << m_state->playerId()
             << ", lastPlayPlayerId(before) =" << m_state->lastPlayPlayerId();

    if (playerId == m_state->lastPlayPlayerId()) {
        // 服务端在“两人连续不要”后会进入新一轮，并把出牌权交还给上一手真正出牌的人。
        // 这里仅重置客户端的 lastPlayPlayerId，让下一次 player_played 被当作“新一轮首出”。
        // 桌面清理放到 onPlayerPlayed（首出）里做，避免刚出现的“不要”提示被立刻清掉。
        m_state->setLastPlayPlayerId(-1);
        qDebug() << "[client onPlayTurn] reset lastPlayPlayerId to -1 (round reset)";
    }

    qDebug() << "[client onPlayTurn]"
             << "lastPlayPlayerId(after) =" << m_state->lastPlayPlayerId();

    m_panel->handlePlayTurn(playerId);
}

void GameFlowController::onPlayerPlayed(int playerId, Cards cards)
{
    Player *player = resolvePlayer(playerId);
    if (player == nullptr) {
        return;
    }

    int oldLastPlayPlayerId = m_state->lastPlayPlayerId();
    Player *lastPlayer = (oldLastPlayPlayerId == -1)
        ? nullptr
        : resolvePlayer(oldLastPlayPlayerId);

    // 新一轮首出：先清掉上一轮桌面（包括两次不要的提示），再渲染这一手
    if (oldLastPlayPlayerId == -1) {
        m_panel->clearAllPlayerDropCards();
    }

    if (playerId == m_state->playerId()) {
        Cards remain = player->getCards();
        remain.remove(cards);
        player->setCards(remain);
    }
    else
    {
        // 别人出牌：剩余牌数递减（用于渲染牌背）
        int cur = m_panel->remainCount(player);
        if (cur <= 0) cur = 17; // 兜底
        m_panel->setRemainCount(player, cur - cards.cardCount());
    }

    m_panel->handlePlayerPlayed(player, lastPlayer, playerId, cards);

    m_state->setLastPlayPlayerId(playerId);
}

void GameFlowController::onPlayerPassed(int playerId)
{
    Player* player = resolvePlayer(playerId);
    m_panel->handlePlayerPassed(player, playerId);
}

void GameFlowController::onGameOver(int winnerId,
                                    int lordPlayerId,
                                    bool lordWin,
                                    int score1,
                                    int score2,
                                    int score3)
{
    Q_UNUSED(winnerId);

    Player* p1 = resolvePlayer(1);
    Player* p2 = resolvePlayer(2);
    Player* p3 = resolvePlayer(3);

    if (p1 == nullptr || p2 == nullptr || p3 == nullptr) {
        return;
    }

    // 角色再收口一次，避免客户端本地状态和服务端不一致
    p1->setRole(1 == lordPlayerId ? Player::Lord : Player::Farmer);
    p2->setRole(2 == lordPlayerId ? Player::Lord : Player::Farmer);
    p3->setRole(3 == lordPlayerId ? Player::Lord : Player::Farmer);

    // 胜负
    p1->setWin((p1->getRole() == Player::Lord) ? lordWin : !lordWin);
    p2->setWin((p2->getRole() == Player::Lord) ? lordWin : !lordWin);
    p3->setWin((p3->getRole() == Player::Lord) ? lordWin : !lordWin);

    // 累计分
    p1->setScore(p1->getScore() + score1);
    p2->setScore(p2->getScore() + score2);
    p3->setScore(p3->getScore() + score3);

    m_panel->handleGameOver(winnerId);
}

void GameFlowController::onPlayRequested(const Cards &cards)
{
    qDebug() << "[client onPlayRequested]"
             << "playerId =" << m_state->playerId()
             << ", cardCount =" << cards.cardCount();

    m_net->sendPlayCards(m_state->playerId(), cards);
}
void GameFlowController::onPassRequested()
{
    m_net->sendPass(m_state->playerId());
}

void GameFlowController::onCallScoreRequested(int score)
{
    m_net->sendCallScore(score, m_state->playerId());
}

void GameFlowController::onRestartRequested()
{
    m_net->sendRequestRestart(m_state->playerId());
}

Player *GameFlowController::resolvePlayer(int playerId) const
{
    int myId = m_state->playerId();

        if (myId == 1) {
            if (playerId == 1) return m_gameCtl->getUserPlayer();
            if (playerId == 2) return m_gameCtl->getRightRobot();
            if (playerId == 3) return m_gameCtl->getLeftRobot();
        }
        else if (myId == 2) {
            if (playerId == 2) return m_gameCtl->getUserPlayer();
            if (playerId == 3) return m_gameCtl->getRightRobot();
            if (playerId == 1) return m_gameCtl->getLeftRobot();
        }
        else if (myId == 3) {
            if (playerId == 3) return m_gameCtl->getUserPlayer();
            if (playerId == 1) return m_gameCtl->getRightRobot();
            if (playerId == 2) return m_gameCtl->getLeftRobot();
        }

        return nullptr;
}
