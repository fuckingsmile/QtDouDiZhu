#ifndef GAMEPANEL_H
#define GAMEPANEL_H

#include <QLabel>
#include <QMainWindow>
#include <QMap>
#include "cardpanel.h"
#include "gamecontrol.h"
#include "animationwindow.h"
#include "player.h"
#include "countdown.h"
#include "bgmcontrol.h"
#include "endingpanel.h"
#include"gamestate.h"
class GameNetBinder;
class GameFlowController;
QT_BEGIN_NAMESPACE
namespace Ui { class GamePanel; }
QT_END_NAMESPACE

class GamePanel : public QMainWindow
{
    Q_OBJECT

public:
    GamePanel(QWidget *parent = nullptr);
    ~GamePanel();
    enum GameStatus{DispatchCard, CallingLord, PlayingHand};
    enum AnimationType{ShunZi, LianDui, Plane, JokerBomb, Bomb, Bet};
    void setRemainCount(Player* player, int count);
    int remainCount(Player* player) const;
    // 初始化游戏控制类信息
    void gameControlInit();
    // 更新分数面板的分数
    void updatePlayerScore();
    // 切割并存储图片
    void initCardMap();
    // 裁剪图片
    void cropImage(QPixmap& pix, int x, int y, Card& c);
    // 初始化游戏按钮组
    void initButtonsGroup();
    // 初始化玩家在窗口中的上下文环境
    void initPlayerContext();
    // 初始化游戏场景
    void initGameScene();
    // 发牌
    void startDispatchCard();
    // 移动扑克牌
    void cardMoveStep(Player* player, int curPos);
    // 处理分发得到的扑克牌
    void disposeCard(Player* player, const Cards& cards);
    // 更新扑克牌在窗口中的显示
    void updatePlayerCards(Player* player);
    // 加载玩家头像
    QPixmap loadRoleImage(Player::Sex sex, Player::Direction direct, Player::Role role);

    // 定时器的处理动作
    void onDispatchCard();
    // 处理玩家抢地主
    void onGrabLordBet(Player* player, int bet, bool flag);
    // 处理玩家的出牌
    void onDisposePlayHand(Player *player, Player *lastPlayer, Cards cards);
    // 处理玩家选牌
    void onCardSelected(Qt::MouseButton button);
    // 显示特效动画
    void showAnimation(AnimationType type, int bet = 0);
    // 隐藏玩家打出的牌
    void hidePlayerDropCards(Player* player);
    // 显示玩家的最终得分
    void showEndingScorePanel();
    // 初始化闹钟倒计时
    void initCountDown();

    void updatePlayerRoleImages();

    void raiseOverlayWidgets();

    void clearAllPlayerDropCards();
protected:
    void paintEvent(QPaintEvent* ev);
    void mouseMoveEvent(QMouseEvent* ev);

signals:
    void playRequested(const Cards &cards);
    void passRequested();
    void callScoreRequested(int score);
    void restartRequested();

public slots:
    void handleConnected();
    void handleRoomInfo(int count);
    void handleStartGame();

    void handleGameOver(int winnerId);

    void handleWelcome(int playerId);
    void handleDealCards(Cards cards, Cards bottomCards);

    void handleLordConfirmed(Player* lordPlayer, int lordPlayerId, Cards bottomCards);
    void handlePlayerPlayed(Player* player, Player* lastPlayer, int playerId, Cards cards);

    void handleCallTurn(int playerId);

    void handlePlayerCalled(Player* player, int playerId, int score);
    void handlePlayTurn(int playerId);
    void handlePlayerPassed(Player* player, int playerId);

private:
    void clearSelectedCards();
    enum CardAlign{Horizontal, Vertical};
    struct PlayerContext
    {
        // 1. 玩家扑克牌显示的区域
        QRect cardRect;
        // 2. 出牌的区域
        QRect playHandRect;
        // 3. 扑克牌的对齐方式(水平 or 垂直)
        CardAlign align;
        // 4. 扑克牌显示正面还是背面
        bool isFrontSide;
        // 5. 游戏过程中的提示信息, 比如: 不出
        QLabel* info;
        // 6. 玩家的头像
        QLabel* roleImg;
        // 7. 玩家刚打出的牌
        Cards lastCards;
        // 8. 网络版：用于显示对手“剩余牌背”的占位面板（不依赖真实手牌）
        QVector<CardPanel*> backPanels;
    };

    Ui::GamePanel *ui;
    QPixmap m_bkImage;
    GameControl* m_gameCtl;
    QVector<Player*> m_playerList;
    QMap<Card, CardPanel*> m_cardMap;
    QSize m_cardSize;
    QPixmap m_cardBackImg;
    QMap<Player*, PlayerContext> m_contextMap;
    CardPanel* m_baseCard;
    CardPanel* m_moveCard;
    QVector<CardPanel*> m_last3Card;
    QPoint m_baseCardPos;
    GameStatus m_gameStatus = DispatchCard;
    QTimer* m_timer;
    int m_dealAnimSeatIndex = 0;
    int m_dealAnimCount = 0;
    int m_dealAnimMovePos = 0;
    AnimationWindow* m_animation;
    CardPanel* m_curSelCard;
    QSet<CardPanel*> m_selectCards;
    QRect m_cardsRect;
    QHash<CardPanel*, QRect> m_userCards;
    CountDown* m_countDown;
    BGMControl* m_bgm;

    GameNetBinder *m_net;
   GameFlowController *m_flow;

    GameState* m_state;
    void finishNetDealCards();
    EndingPanel* m_endingPanel = nullptr;
    QMap<Player*, int> m_remainCount; // 对手剩余张数（我方可不依赖此值）



};
#endif // GAMEPANEL_H
