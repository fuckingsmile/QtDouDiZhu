#include "endingpanel.h"
#include "gamepanel.h"
#include "playhand.h"
#include "ui_gamepanel.h"
#include <QImage>
#include <QMouseEvent>
#include <QPainter>
#include <QRandomGenerator>
#include <QTimer>
#include <QDebug>
#include <QPropertyAnimation>
#include "gamenetbinder.h"
#include "gameflowcontroller.h"
#include "gamestate.h"
GamePanel::GamePanel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::GamePanel)
{
    ui->setupUi(this);


    ui->verticalLayout->removeWidget(ui->btnGroup);
    ui->btnGroup->setParent(this);

    ui->btnGroup->adjustSize();   //让Qt重新计算真实宽度

    ui->btnGroup->show();
    ui->btnGroup->raise();

    ui->btnGroup->move((width() - ui->btnGroup->width()) / 2 +100, height() - 170);
    m_net = new GameNetBinder(this);
    m_state = new GameState(this);
    m_state->setPlayerId(-1);
    // 1. 背景图
    int num = QRandomGenerator::global()->bounded(10);
    QString path = QString(":/images/background-%1.png").arg(num+1);
    m_bkImage.load(path);

    // 2. 窗口的标题的大小
    this->setWindowTitle("欢乐斗地主");
    this->setFixedSize(1000, 650);

    // 3. 实例化游戏控制类对象
    gameControlInit();

    // 4. 玩家得分(更新)
    updatePlayerScore();

    // 5. 切割游戏图片
    initCardMap();

    // 6. 初始化游戏中的按钮组
    initButtonsGroup();

    // 7. 初始化玩家在窗口中的上下文环境
    initPlayerContext();

    // 8. 扑克牌场景初始化
    initGameScene();

    // 9. 倒计时窗口初始化
    initCountDown();

    // 定时器实例化
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &GamePanel::onDispatchCard);
    m_animation = new AnimationWindow(this);
    m_bgm = new BGMControl(this);

    m_flow = new GameFlowController(m_net, m_state, m_gameCtl, this, this);

    m_net->connectToServer("127.0.0.1", 9999);


}

GamePanel::~GamePanel()
{
    delete ui;
}

void GamePanel::setRemainCount(Player *player, int count)
{
    if (player == nullptr) return;
    m_remainCount[player] = qMax(0, count);
    if (player != m_gameCtl->getUserPlayer())
    {
        updatePlayerCards(player);
    }
}

int GamePanel::remainCount(Player *player) const
{
    if (player == nullptr) return 0;
    return m_remainCount.value(player, 0);
}

void GamePanel::gameControlInit()
{
    m_gameCtl = new GameControl(this);
    m_gameCtl->playerInit();
    // 得到三个玩家的实例对象
    Robot* leftRobot = m_gameCtl->getLeftRobot();
    Robot* rightRobot = m_gameCtl->getRightRobot();
    UserPlayer* user = m_gameCtl->getUserPlayer();
    // 存储的顺序: 左侧机器人, 右侧机器人, 当前玩家
    m_playerList << leftRobot << rightRobot << user;
    connect(leftRobot, &Player::notifyPickCards, this, &GamePanel::disposeCard);
    connect(rightRobot, &Player::notifyPickCards, this, &GamePanel::disposeCard);
    connect(user, &Player::notifyPickCards, this, &GamePanel::disposeCard);
}

void GamePanel::updatePlayerScore()
{
    ui->scorePanel->setScores(
    m_playerList[0]->getScore(),
    m_playerList[1]->getScore(),
    m_playerList[2]->getScore());
}

void GamePanel::initCardMap()
{
    // 1. 加载大图
    QPixmap pixmap(":/images/card.png");
    // 2. 计算每张图片大小
    m_cardSize.setWidth(pixmap.width() / 13);
    m_cardSize.setHeight(pixmap.height() / 5);

    // 3. 背景图
    m_cardBackImg = pixmap.copy(2*m_cardSize.width(), 4*m_cardSize.height(),
                                m_cardSize.width(), m_cardSize.height());
    // 正常花色
    for(int i=0, suit=Card::Suit_Begin+1; suit<Card::Suit_End; ++suit, ++i)
    {
        for(int j=0, pt=Card::Card_Begin+1; pt<Card::Card_SJ; ++pt, ++j)
        {
            Card card((Card::CardPoint)pt, (Card::CardSuit)suit);
            // 裁剪图片
            cropImage(pixmap, j*m_cardSize.width(), i*m_cardSize.height(), card);
        }
    }
    // 大小王
    Card c;
    c.setPoint(Card::Card_SJ);
    c.setSuit(Card::Suit_Begin);
    cropImage(pixmap, 0, 4*m_cardSize.height(), c);

    c.setPoint(Card::Card_BJ);
    cropImage(pixmap, m_cardSize.width(), 4*m_cardSize.height(), c);
}

void GamePanel::cropImage(QPixmap &pix, int x, int y, Card& c)
{
    QPixmap sub = pix.copy(x, y, m_cardSize.width(), m_cardSize.height());
    CardPanel* panel = new CardPanel(this);
    panel->setImage(sub, m_cardBackImg);
    panel->setCard(c);
    panel->hide();
    m_cardMap.insert(c, panel);
    connect(panel, &CardPanel::cardSelected, this, &GamePanel::onCardSelected);
}

void GamePanel::initButtonsGroup()
{
    ui->btnGroup->initButtons();
    ui->btnGroup->selectPanel(ButtonGroup::Empty);
    ui->btnGroup->hide();

    connect(ui->btnGroup, &ButtonGroup::playHand, this, [=]() {

        qDebug() << "[client click play] selected count =" << m_selectCards.size()
                 << ", lastPlayPlayerId =" << m_state->lastPlayPlayerId()
                 << ", myId =" << m_state->playerId();

        if (m_selectCards.isEmpty()) {
            qDebug() << "[client click play] no selected cards";
            return;
        }

        Cards cards;
        for (CardPanel* panel : m_selectCards) {
            if (panel != nullptr) {
                cards.add(panel->getCard());
            }
        }

        PlayHand hand(cards);
        qDebug() << "[client click play] hand type =" << hand.getHandType()
                 << ", card count =" << cards.cardCount();

        if (hand.getHandType() == PlayHand::Hand_Unknown) {
            qDebug() << "[client click play] hand unknown, blocked locally";
            return;
        }

        qDebug() << "[client click play] emit playRequested";
        emit playRequested(cards);
    });

    connect(ui->btnGroup, &ButtonGroup::pass, this, [=]() {
       emit passRequested();
        ui->btnGroup->hide();
        m_countDown->stopCountDown();
    });
    connect(ui->btnGroup, &ButtonGroup::betPoint, this, [=](int bet){
      emit callScoreRequested(bet);
        ui->btnGroup->selectPanel(ButtonGroup::Empty);
    });
}

void GamePanel::initPlayerContext()
{
    // 1. 放置玩家扑克牌的区域
    const QRect cardsRect[] =
    {
        QRect(90, 130, 100, height() - 200),                    // 左侧机器人
        QRect(rect().right() - 190, 130, 100, height() - 200),  // 右侧机器人
        QRect(80, rect().bottom() - 150, width() - 160, 120)    // 当前玩家
    };
    const QRect playHandRect[] =
    {
        QRect(260, 150, 100, 100),                              // 左侧机器人
        QRect(rect().right() - 360, 150, 100, 100),             // 右侧机器人
       QRect(120, rect().bottom() - 300, width() - 240, 110)   // 当前玩家
    };
    const QPoint roleImgPos[] =
    {
        QPoint(cardsRect[0].left()-80, cardsRect[0].height() / 2 + 20),     // 左侧机器人
        QPoint(cardsRect[1].right()+10, cardsRect[1].height() / 2 + 20),    // 右侧机器人
        QPoint(cardsRect[2].right()-10, cardsRect[2].top() - 10)            // 当前玩家
    };

    // 循环
    int index = m_playerList.indexOf(m_gameCtl->getUserPlayer());
    for(int i=0; i<m_playerList.size(); ++i)
    {
        PlayerContext context;
        context.align = i==index ? Horizontal : Vertical;
        context.isFrontSide = i==index ? true : false;
        context.cardRect = cardsRect[i];
        context.playHandRect = playHandRect[i];
        // 提示信息
        context.info = new QLabel(this);
        context.info->resize(160, 98);
        context.info->hide();
        // 显示到出牌区域的中心位置
        QRect rect = playHandRect[i];
        QPoint pt(rect.left() + (rect.width() - context.info->width()) / 2,
                  rect.top() + (rect.height() - context.info->height()) / 2);
        context.info->move(pt);
        // 玩家的头像
        context.roleImg = new QLabel(this);
        context.roleImg->resize(84, 120);
        context.roleImg->hide();
        context.roleImg->move(roleImgPos[i]);
        m_contextMap.insert(m_playerList.at(i), context);
    }
}

void GamePanel::initGameScene()
{
    // 1. 发牌区的扑克牌
    m_baseCard = new CardPanel(this);
    m_baseCard->setImage(m_cardBackImg, m_cardBackImg);
    // 2. 发牌过程中移动的扑克牌
    m_moveCard = new CardPanel(this);
    m_moveCard->setImage(m_cardBackImg, m_cardBackImg);
    // 3. 最后的三张底牌(用于窗口的显示)
    for(int i=0; i<3; ++i)
    {
        CardPanel* panel = new CardPanel(this);
        panel->setImage(m_cardBackImg, m_cardBackImg);
        m_last3Card.push_back(panel);
        panel->hide();
    }
    // 扑克牌的位置
    m_baseCardPos = QPoint((width() - m_cardSize.width()) / 2,
                           height() / 2 - 100);
    m_baseCard->move(m_baseCardPos);
    m_moveCard->move(m_baseCardPos);

    int base = (width() - 3 * m_cardSize.width() - 2 * 10) / 2;
    for(int i=0; i<3; ++i)
    {
        m_last3Card[i]->move(base + (m_cardSize.width() + 10) * i, 20);
    }
}


void GamePanel::initCountDown()
{
    m_countDown = new CountDown(this);
    m_countDown->move((width() - m_countDown->width()) / 2, (height() - m_countDown->height()) / 2 + 30);
    connect(m_countDown, &CountDown::notMuchTime, this, [=](){
        // 播放提示音乐
        m_bgm->playAssistMusic(BGMControl::Alert);
    });
    connect(m_countDown, &CountDown::timeout, this, [=]() {

        if (m_gameStatus != PlayingHand) {
            return;
        }

        // 首出时不能不要，先不自动处理
        if (m_state->lastPlayPlayerId() == -1 || m_state->lastPlayPlayerId() == m_state->playerId()) {
            qDebug() << "timeout: must play, cannot pass";
            return;
        }

        // 跟牌阶段，超时默认不要
       emit passRequested();

        ui->btnGroup->hide();
        m_countDown->stopCountDown();
    });
}

void GamePanel::updatePlayerRoleImages()
{
    for (Player* player : m_playerList)
        {
            PlayerContext &context = m_contextMap[player];

            QPixmap pix = loadRoleImage(player->getSex(),
                                        player->getDirection(),
                                        player->getRole());

            context.roleImg->setPixmap(pix);
            context.roleImg->setScaledContents(true);
            context.roleImg->show();
            context.roleImg->raise();
        }
}



void GamePanel::handleWelcome(int playerId)
{
    this->setWindowTitle(QString("Landlords - 我是玩家 %1").arg(playerId));
}

void GamePanel::handleDealCards(Cards cards, Cards bottomCards)
{
    Q_UNUSED(cards);

    CardList bottomList = bottomCards.toCardList();
    for (int i = 0; i < m_last3Card.size() && i < bottomList.size(); ++i)
    {
        Card card = bottomList.at(i);

        m_last3Card[i]->setCard(card);

        if (m_cardMap.contains(card) && m_cardMap[card] != nullptr) {
            QPixmap front = m_cardMap[card]->getImage();
            m_last3Card[i]->setImage(front, m_cardBackImg);
        }

        m_last3Card[i]->setFrontSide(true);
        m_last3Card[i]->show();
        m_last3Card[i]->raise();
        m_last3Card[i]->update();
    }
}

void GamePanel::handleLordConfirmed(Player* lordPlayer, int lordPlayerId, Cards bottomCards)
{
    if (lordPlayer == nullptr) {
        return;
    }

    // 地主确认后，叫/抢地主阶段的提示图不应继续显示
    for (Player* p : m_playerList) {
        auto it = m_contextMap.find(p);
        if (it != m_contextMap.end() && it->info != nullptr) {
            it->info->hide();
        }
    }

    CardList bottomList = bottomCards.toCardList();
    for (int i = 0; i < m_last3Card.size() && i < bottomList.size(); ++i)
    {
        m_last3Card[i]->setCard(bottomList.at(i));
        m_last3Card[i]->setFrontSide(true);
        m_last3Card[i]->update();
    }

    if (lordPlayerId == m_state->playerId())
    {
        disposeCard(lordPlayer, lordPlayer->getCards());
    }
    else
    {
        updatePlayerCards(lordPlayer);
    }

    updatePlayerScore();
    update();
    updatePlayerRoleImages();
}

void GamePanel::handlePlayerPlayed(Player* player, Player* lastPlayer, int playerId, Cards cards)
{
    if (player == nullptr) {
        return;
    }

    if (playerId == m_state->playerId())
    {
        clearSelectedCards();
    }

    onDisposePlayHand(player, lastPlayer, cards);

    if (playerId == m_state->playerId())
    {
        disposeCard(player, player->getCards());
    }
    else
    {
        updatePlayerCards(player);
    }

    update();
    raiseOverlayWidgets();
}

void GamePanel::handleCallTurn(int playerId)
{
    m_gameStatus = CallingLord;
    if (playerId == m_state->playerId()) {

        ui->btnGroup->selectPanel(ButtonGroup::CallLord, m_state->currentCallScore());
         // 把按钮组移动到手牌上方
        ui->btnGroup->move((width() - ui->btnGroup->width()) / 2, height() - 230);
         ui->btnGroup->raise();
         ui->btnGroup->show();

         this->setWindowTitle(QString("Landlords - 轮到我叫地主"));
     }
     else {
         ui->btnGroup->selectPanel(ButtonGroup::Empty);
         this->setWindowTitle(QString("Landlords - 轮到玩家 %1 叫地主").arg(playerId));
     }

}

void GamePanel::handlePlayerCalled(Player* player, int playerId, int score)
{
    this->setWindowTitle(
        QString("Landlords - 玩家 %1 叫了 %2 分")
            .arg(playerId)
            .arg(score));

    if (player == nullptr) {
        return;
    }

    bool isFirst = (score > 0 && m_state->currentCallScore() == 0);
    onGrabLordBet(player, score, isFirst);
}

void GamePanel::handlePlayTurn(int playerId)
{
    qDebug() << "[client handlePlayTurn]"
                << "turnPlayerId =" << playerId
                << ", myId =" << m_state->playerId()
                << ", lastPlayPlayerId =" << m_state->lastPlayPlayerId();

       // 只在“首次进入出牌阶段”时清一次叫地主/抢地主提示
       const bool firstEnterPlaying = (m_gameStatus != PlayingHand);
       m_gameStatus = PlayingHand;
       if (firstEnterPlaying) {
           for (Player* p : m_playerList) {
               auto it = m_contextMap.find(p);
               if (it != m_contextMap.end() && it->info != nullptr) {
                   it->info->hide();
               }
           }
       }

       if (playerId == m_state->playerId()) {

           if (m_state->lastPlayPlayerId() == -1 ||
               m_state->lastPlayPlayerId() == m_state->playerId()) {
               qDebug() << "[client handlePlayTurn] use PlayCard panel";
               ui->btnGroup->selectPanel(ButtonGroup::PlayCard);
           } else {
               qDebug() << "[client handlePlayTurn] use PassOrPlay panel";
               ui->btnGroup->selectPanel(ButtonGroup::PassOrPlay);
           }

           ui->btnGroup->move((width() - ui->btnGroup->width()) / 2, height() - 230);
           ui->btnGroup->show();
           ui->btnGroup->raise();

           m_countDown->showCountDown();
           m_countDown->raise();
           m_countDown->show();
           this->setWindowTitle("Landlords - 轮到我出牌");
       } else {
           qDebug() << "[client handlePlayTurn] not my turn";

           ui->btnGroup->selectPanel(ButtonGroup::Empty);
           ui->btnGroup->hide();
           m_countDown->stopCountDown();
           m_countDown->hide();
           this->setWindowTitle(QString("Landlords - 轮到玩家 %1 出牌").arg(playerId));
       }
    raiseOverlayWidgets();
}

void GamePanel::handlePlayerPassed(Player* player, int playerId)
{
    this->setWindowTitle(
        QString("Landlords - 玩家 %1 不要")
            .arg(playerId));
    if (player == nullptr) {
        return;
    }
    Cards emptyCards;
    onDisposePlayHand(player, nullptr, emptyCards);
    //raiseOverlayWidgets();
}

void GamePanel::startDispatchCard()
{
    for(auto it = m_cardMap.begin(); it!= m_cardMap.end(); ++it)
    {
        it.value()->setSeclected(false);
        it.value()->setFrontSide(true);
        it.value()->hide();
    }

    for(int i=0; i<m_last3Card.size(); ++i)
    {
        m_last3Card.at(i)->hide();
    }

    int index = m_playerList.indexOf(m_gameCtl->getUserPlayer());
    for(int i=0; i<m_playerList.size(); ++i)
    {
        m_contextMap[m_playerList.at(i)].lastCards.clear();
        m_contextMap[m_playerList.at(i)].info->hide();
        m_contextMap[m_playerList.at(i)].roleImg->hide();
        m_contextMap[m_playerList.at(i)].isFrontSide = i==index ? true : false;
    }

    m_baseCard->show();
    ui->btnGroup->selectPanel(ButtonGroup::Empty);

    m_dealAnimSeatIndex = 0;
    m_dealAnimCount = 0;
    m_dealAnimMovePos = 0;

    m_timer->start(10);
    m_bgm->playAssistMusic(BGMControl::Dispatch);
}
void GamePanel::cardMoveStep(Player *player, int curPos)
{
    // 得到每个玩家的扑克牌展示区域
    QRect cardRect = m_contextMap[player].cardRect;
    // 每个玩家的单元步长
    const int unit[] = {
        (m_baseCardPos.x() - cardRect.right()) / 100,
        (cardRect.left() - m_baseCardPos.x()) / 100,
        (cardRect.top() - m_baseCardPos.y()) / 100
    };
    // 每次窗口移动的时候每个玩家对应的牌的时时坐标位置
    const QPoint pos[] =
    {
        QPoint(m_baseCardPos.x() - curPos * unit[0], m_baseCardPos.y()),
        QPoint(m_baseCardPos.x() + curPos * unit[1], m_baseCardPos.y()),
        QPoint(m_baseCardPos.x(), m_baseCardPos.y() + curPos * unit[2]),
    };

    // 移动扑克牌窗口
    int index = m_playerList.indexOf(player);
    m_moveCard->move(pos[index]);

    // 临界状态处理
    if(curPos == 0)
    {
        m_moveCard->show();
    }
    if(curPos == 100)
    {
        m_moveCard->hide();
    }
}

void GamePanel::disposeCard(Player *player, const Cards &cards)
{
    Cards& myCard = const_cast<Cards&>(cards);
    CardList list = myCard.toCardList();

    for (int i = 0; i < list.size(); ++i)
    {
        Card card = list.at(i);

        if (!m_cardMap.contains(card)) {
            qDebug() << "disposeCard: card not found in m_cardMap";
            continue;
        }

        CardPanel* panel = m_cardMap[card];
        if (panel == nullptr) {
            qDebug() << "disposeCard: panel is nullptr";
            continue;
        }

        panel->setOwner(player);
    }

    updatePlayerCards(player);
}

void GamePanel::updatePlayerCards(Player *player)
{
    Cards cards = player->getCards();
    CardList list = cards.toCardList();

    m_cardsRect = QRect();
    m_userCards.clear();

    int cardSpace = 25;
    QRect cardsRect = m_contextMap[player].cardRect;

    // 网络版：对手不持有真实手牌时，用“剩余张数”渲染一列牌背
    if (player != m_gameCtl->getUserPlayer() && !m_contextMap[player].isFrontSide)
    {
        int remain = m_remainCount.value(player, 0);

        // 隐藏之前通过真实 Card 映射出来的手牌面板（理论上对手不会走到这里，但做一下兜底）
        for (int i = 0; i < list.size(); ++i)
        {
            Card card = list.at(i);
            if (m_cardMap.contains(card) && m_cardMap[card] != nullptr)
            {
                m_cardMap[card]->hide();
            }
        }

        PlayerContext &ctx = m_contextMap[player];

        // 调整 backPanels 数量
        while (ctx.backPanels.size() > remain)
        {
            CardPanel *p = ctx.backPanels.takeLast();
            if (p) p->hide();
        }
        while (ctx.backPanels.size() < remain)
        {
            CardPanel *p = new CardPanel(this);
            p->setImage(m_cardBackImg, m_cardBackImg);
            p->setFrontSide(false);
            p->setOwner(player);
            p->show();
            p->raise();
            ctx.backPanels.push_back(p);
        }

        // 垂直摆放：根据可用高度自适应间距，确保 17 张能放下
        const int minSpacing = 8;
        const int maxSpacing = 18;
        int spacing = maxSpacing;
        if (remain > 1) {
            const int maxSpan = qMax(0, cardsRect.height() - m_cardSize.height());
            spacing = maxSpan / (remain - 1);
            spacing = qBound(minSpacing, spacing, maxSpacing);
        }

        int leftX = cardsRect.left() + (cardsRect.width() - m_cardSize.width()) / 2;
        int topY = cardsRect.top() +
                   (cardsRect.height() - ((remain > 0 ? (remain - 1) * spacing : 0) + m_cardSize.height())) / 2;

        for (int i = 0; i < ctx.backPanels.size(); ++i)
        {
            CardPanel *p = ctx.backPanels[i];
            if (!p) continue;
            p->setImage(m_cardBackImg, m_cardBackImg);
            p->setFrontSide(false);
            p->move(leftX, topY + i * spacing);
            p->show();
            p->raise();
        }

        raiseOverlayWidgets();
        return;
    }

    for (int i = 0; i < list.size(); ++i)
    {
        Card card = list.at(i);

        if (!m_cardMap.contains(card)) {
            qDebug() << "updatePlayerCards: hand card not found in m_cardMap";
            continue;
        }

        CardPanel* panel = m_cardMap[card];
        if (panel == nullptr) {
            qDebug() << "updatePlayerCards: hand panel is nullptr";
            continue;
        }

        panel->show();
        panel->raise();
        panel->setFrontSide(m_contextMap[player].isFrontSide);

        if (m_contextMap[player].align == Horizontal)
        {
            int leftX = cardsRect.left() +
                    (cardsRect.width() - (list.size() - 1) * cardSpace - panel->width()) / 2;
            int topY = cardsRect.top() +
                    (cardsRect.height() - m_cardSize.height()) / 2;

            if (player == m_gameCtl->getUserPlayer())
            {
                topY -= 25;
            }

            if (panel->isSelected())
            {
                topY -= 10;
            }

            panel->move(leftX + cardSpace * i, topY);
            m_cardsRect = QRect(leftX, topY,
                                cardSpace * i + m_cardSize.width(),
                                m_cardSize.height());

            int curWidth = 0;
            if (list.size() - 1 == i)
            {
                curWidth = m_cardSize.width();
            }
            else
            {
                curWidth = cardSpace;
            }

            QRect cardRect(leftX + cardSpace * i, topY, curWidth, m_cardSize.height());
            m_userCards.insert(panel, cardRect);
        }
        else
        {
            int leftX = cardsRect.left() + (cardsRect.width() - m_cardSize.width()) / 2;
            int topY = cardsRect.top() +
                    (cardsRect.height() - (list.size() - 1) * cardSpace - panel->height()) / 2;
            panel->move(leftX, topY + i * cardSpace);
        }
    }

    QRect playCardRect = m_contextMap[player].playHandRect;
    Cards lastCards = m_contextMap[player].lastCards;
    if (!lastCards.isEmpty())
    {
        int playSpacing = 24;
        CardList lastCardList = lastCards.toCardList();
        CardList::ConstIterator itplayed = lastCardList.constBegin();

        for (int i = 0; itplayed != lastCardList.constEnd(); ++itplayed, ++i)
        {
            Card card = *itplayed;

            if (!m_cardMap.contains(card)) {
                qDebug() << "updatePlayerCards: played card not found in m_cardMap";
                continue;
            }

            CardPanel* panel = m_cardMap[card];
            if (panel == nullptr) {
                qDebug() << "updatePlayerCards: played panel is nullptr";
                continue;
            }

            panel->setFrontSide(true);
            panel->raise();

            if (m_contextMap[player].align == Horizontal)
            {
                int leftBase = playCardRect.left() +
                        (playCardRect.width() - (lastCardList.size() - 1) * playSpacing - panel->width()) / 2;
                int top = playCardRect.top() + (playCardRect.height() - panel->height()) / 2;
                panel->move(leftBase + i * playSpacing, top);
            }
            else
            {
                int left = playCardRect.left() + (playCardRect.width() - panel->width()) / 2;
                int top = playCardRect.top();
                panel->move(left, top + i * playSpacing);
            }

            panel->show();
        }
    }
    raiseOverlayWidgets();
}

QPixmap GamePanel::loadRoleImage(Player::Sex sex, Player::Direction direct, Player::Role role)
{
    // 找图片
    QVector<QString> lordMan;
    QVector<QString> lordWoman;
    QVector<QString> farmerMan;
    QVector<QString> farmerWoman;
    lordMan << ":/images/lord_man_1.png" << ":/images/lord_man_2.png";
    lordWoman << ":/images/lord_woman_1.png" << ":/images/lord_woman_2.png";
    farmerMan << ":/images/farmer_man_1.png" << ":/images/farmer_man_2.png";
    farmerWoman << ":/images/farmer_woman_1.png" << ":/images/farmer_woman_2.png";

    // 加载图片  QImage
    QImage image;
    int random = QRandomGenerator::global()->bounded(2);
    if(sex == Player::Man && role == Player::Lord)
    {
        image.load(lordMan.at(random));
    }
    else if(sex == Player::Man && role == Player::Farmer)
    {
        image.load(farmerMan.at(random));
    }
    else if(sex == Player::Woman && role == Player::Lord)
    {
        image.load(lordWoman.at(random));
    }
    else if(sex == Player::Woman && role == Player::Farmer)
    {
        image.load(farmerWoman.at(random));
    }

    QPixmap pixmap;
    if(direct == Player::Left)
    {
        pixmap = QPixmap::fromImage(image);
    }
    else
    {
        pixmap = QPixmap::fromImage(image.mirrored(true, false));
    }
    return pixmap;
}

void GamePanel::onDispatchCard()
{
    if (m_playerList.size() < 3) {
        m_timer->stop();
        m_bgm->stopAssistMusic();
        m_moveCard->hide();
        m_baseCard->hide();
        m_dealAnimSeatIndex = 0;
        m_dealAnimCount = 0;
        m_dealAnimMovePos = 0;
        return;
    }

    Player* targetPlayer = m_playerList.at(m_dealAnimSeatIndex);
    if (targetPlayer == nullptr) {
        m_timer->stop();
        m_bgm->stopAssistMusic();
        m_moveCard->hide();
        m_baseCard->hide();
        m_dealAnimSeatIndex = 0;
        m_dealAnimCount = 0;
        m_dealAnimMovePos = 0;
        return;
    }

    // 先走当前这一步动画
    cardMoveStep(targetPlayer, m_dealAnimMovePos);
    m_dealAnimMovePos += 15;

    // 一张牌飞完
    if (m_dealAnimMovePos >= 100)
    {
        m_dealAnimCount++;
        m_dealAnimSeatIndex = (m_dealAnimSeatIndex + 1) % 3;
        m_dealAnimMovePos = 0;

        // 17 * 3 = 51 张发完
        if (m_dealAnimCount >= 51)
        {
            m_timer->stop();
            m_bgm->stopAssistMusic();

            m_moveCard->hide();
            m_baseCard->hide();

            finishNetDealCards();
            raiseOverlayWidgets();
            update();

            m_dealAnimSeatIndex = 0;
            m_dealAnimCount = 0;
            m_dealAnimMovePos = 0;
            return;
        }
    }
}

void GamePanel::onGrabLordBet(Player *player, int bet, bool flag)
{
    // 显示抢地主的信息提示
    PlayerContext context = m_contextMap[player];
    if(bet == 0)
    {
        context.info->setPixmap(QPixmap(":/images/buqinag.png"));
    }
    else
    {
        if(flag)
        {
            context.info->setPixmap(QPixmap(":/images/jiaodizhu.png"));
        }
        else
        {
            context.info->setPixmap(QPixmap(":/images/qiangdizhu.png"));
        }
        // 显示叫地主的分数
        showAnimation(Bet, bet);
    }
    context.info->show();

    // 播放分数的背景音乐
    m_bgm->playerRobLordMusic(bet, (BGMControl::RoleSex)player->getSex(), flag);
}

void GamePanel::onDisposePlayHand(Player *player, Player *lastPlayer, Cards cards)
{

    if (player == nullptr) {
            return;
        }
    // 存储玩家打出的牌
    auto it = m_contextMap.find(player);
    it->lastCards = cards;
    // 2. 根据牌型播放游戏特效
    PlayHand hand(cards);
    PlayHand::HandType type = hand.getHandType();
    if(type == PlayHand::Hand_Plane ||
            type == PlayHand::Hand_Plane_Two_Pair ||
            type == PlayHand::Hand_Plane_Two_Single)
    {
        showAnimation(Plane);
    }
    else if(type == PlayHand::Hand_Seq_Pair)
    {
        showAnimation(LianDui);
    }
    else if(type == PlayHand::Hand_Seq_Single)
    {
        showAnimation(ShunZi);
    }
    else if(type == PlayHand::Hand_Bomb)
    {
        showAnimation(Bomb);
    }
    else if(type == PlayHand::Hand_Bomb_Jokers)
    {
        showAnimation(JokerBomb);
    }
    // 如果玩家打出的是空牌(不出牌), 显示提示信息
    if(cards.isEmpty())
    {
        it->info->setPixmap(QPixmap(":/images/pass.png"));
        it->info->show();
        m_bgm->playPassMusic((BGMControl::RoleSex)player->getSex());
    }
    else
    {
        bool isFirstHand = (lastPlayer == nullptr || lastPlayer == player);
        m_bgm->playCardMusic(cards, isFirstHand, (BGMControl::RoleSex)player->getSex());
    }
    // 3. 更新玩家剩余的牌
    updatePlayerCards(player);
    // 4. 播放提示音乐
    // 判断玩家剩余的牌的数量
    if(player->getCards().cardCount() == 2)
    {
        m_bgm->playLastMusic(BGMControl::Last2, (BGMControl::RoleSex)player->getSex());
    }
    else if(player->getCards().cardCount() == 1)
    {
        m_bgm->playLastMusic(BGMControl::Last1, (BGMControl::RoleSex)player->getSex());
    }
}

void GamePanel::showAnimation(AnimationType type, int bet)
{
    switch(type)
    {
    case AnimationType::LianDui:
    case AnimationType::ShunZi:
        m_animation->setFixedSize(250, 150);
        m_animation->move((width()-m_animation->width())/2, 200);
        m_animation->showSequence((AnimationWindow::Type)type);
        break;
    case AnimationType::Plane:
        m_animation->setFixedSize(800, 75);
        m_animation->move((width()-m_animation->width())/2, 200);
        m_animation->showPlane();
        break;
    case AnimationType::Bomb:
        m_animation->setFixedSize(180, 200);
        m_animation->move((width()-m_animation->width())/2, (height() - m_animation->height()) / 2 - 70);
        m_animation->showBomb();
        break;
    case AnimationType::JokerBomb:
        m_animation->setFixedSize(250, 200);
        m_animation->move((width()-m_animation->width())/2, (height() - m_animation->height()) / 2 - 70);
        m_animation->showJokerBomb();
        break;
    case AnimationType::Bet:
        m_animation->setFixedSize(160, 98);
        m_animation->move((width()-m_animation->width())/2, (height()-m_animation->height())/2-140);
        m_animation->showBetScore(bet);
        break;
    }
    m_animation->show();
}


void GamePanel::hidePlayerDropCards(Player *player)
{
    auto it = m_contextMap.find(player);
    if(it != m_contextMap.end())
    {
        if(it->lastCards.isEmpty())
        {
            it->info->hide();
        }
        else
        {
            // Cards --> Card
            CardList list = it->lastCards.toCardList();
            for(auto last = list.begin(); last != list.end(); ++last)
            {
                m_cardMap[*last]->hide();
            }
        }
        it->lastCards.clear();
    }
}

void GamePanel::showEndingScorePanel()
{
    bool islord = m_gameCtl->getUserPlayer()->getRole() == Player::Lord ? true : false;
    bool isWin = m_gameCtl->getUserPlayer()->isWin();
    m_endingPanel = new EndingPanel(islord, isWin, this);
    m_endingPanel->show();
    m_endingPanel->move((width() - m_endingPanel->width()) / 2, -m_endingPanel->height());
    m_endingPanel->setPlayerScore(m_gameCtl->getLeftRobot()->getScore(),
                         m_gameCtl->getRightRobot()->getScore(),
                         m_gameCtl->getUserPlayer()->getScore());
    if(isWin)
    {
        m_bgm->playEndingMusic(true);
    }
    else
    {
        m_bgm->playEndingMusic(false);
    }

    QPropertyAnimation *animation = new QPropertyAnimation(m_endingPanel, "geometry", this);
    // 动画持续的时间
    animation->setDuration(1500);   // 1.5s
    // 设置窗口的起始位置和终止位置
    animation->setStartValue(QRect(m_endingPanel->x(), m_endingPanel->y(), m_endingPanel->width(), m_endingPanel->height()));
    animation->setEndValue(QRect((width() - m_endingPanel->width()) / 2, (height() - m_endingPanel->height()) / 2,
                                 m_endingPanel->width(), m_endingPanel->height()));
    // 设置窗口的运动曲线
    animation->setEasingCurve(QEasingCurve(QEasingCurve::OutBounce));
    // 播放动画效果
    animation->start();

    // 处理窗口信号
    connect(m_endingPanel, &EndingPanel::continueGame, this, [=]()
    {
        ui->btnGroup->hide();

        // 通知服务器：我准备继续
        emit restartRequested();
        // 可以先把按钮禁用，避免重复点击
        m_endingPanel->setEnabled(false);
    });
}


void GamePanel::clearAllPlayerDropCards()
{
    for (int i = 0; i < m_playerList.size(); ++i){
            hidePlayerDropCards(m_playerList.at(i));
    }
}

void GamePanel::clearSelectedCards()
{
    for (CardPanel* panel : m_selectCards)
        {
            if (panel != nullptr)
            {
                panel->setSeclected(false);
            }
        }
        m_selectCards.clear();
        m_curSelCard = nullptr;
}

void GamePanel::finishNetDealCards()
{
    Player* user = m_gameCtl->getUserPlayer();
        if (user == nullptr) {
            return;
        }
        // 清掉动画阶段可能留下的本地假牌
        user->clearCards();
        user->setCards(m_state->netCards());
        disposeCard(user, m_state->netCards());
        this->setWindowTitle(
            QString("Landlords - 我是玩家 %1 / 收到 %2 张牌")
                .arg(m_state->playerId())
                .arg(m_state->netCards().cardCount()));
}


void GamePanel::onCardSelected(Qt::MouseButton button)
{
    // 1. 判断是不是出牌状态
    if(m_gameStatus == DispatchCard ||
            m_gameStatus == CallingLord)
    {
        return;
    }
    // 2. 判断发出信号的牌的所有者是不是当前用户玩家
    CardPanel* panel = static_cast<CardPanel*>(sender());
    if(panel->getOwner() != m_gameCtl->getUserPlayer())
    {
        return;
    }
    // 3. 保存当前被选中的牌的窗口对象
    m_curSelCard = panel;
    // 4. 判断参数的鼠标键是左键还是右键
    if(button == Qt::LeftButton)
    {
        panel->setSeclected(!panel->isSelected());
            updatePlayerCards(panel->getOwner());

            QSet<CardPanel*>::const_iterator it = m_selectCards.find(panel);
            if(it == m_selectCards.constEnd())
            {
                m_selectCards.insert(panel);
            }
            else
            {
                m_selectCards.erase(it);
            }

            // 选牌后，手牌会重新排布，重新把按钮组抬到最上层
            if (m_gameStatus == PlayingHand) {
                ui->btnGroup->raise();
                ui->btnGroup->show();
            }

            m_bgm->playAssistMusic(BGMControl::SelectCard);
    }
    else if(button == Qt::RightButton)
    {
        // 网络版下，先不使用右键直接出牌，避免走本地单机逻辑
            return;
    }
}

void GamePanel::paintEvent(QPaintEvent *ev)
{
    Q_UNUSED(ev)
    QPainter p(this);
    p.drawPixmap(rect(), m_bkImage);
}

void GamePanel::mouseMoveEvent(QMouseEvent *ev)
{
    Q_UNUSED(ev)
    if(ev->buttons() & Qt::LeftButton)
    {
        QPoint pt = ev->pos();
        if(!m_cardsRect.contains(pt))
        {
            m_curSelCard = nullptr;
        }
        else
        {
            QList<CardPanel*> list = m_userCards.keys();
            for(int i=0; i<list.size(); ++i)
            {
                CardPanel* panel = list.at(i);
                if(m_userCards[panel].contains(pt) &&m_curSelCard != panel)
                {
                    // 点击这张扑克牌
                    panel->clicked();
                    m_curSelCard = panel;
                }
            }
        }
    }
}

void GamePanel::handleConnected()
{
    this->setWindowTitle("Landlords - 已连接服务器");
}

void GamePanel::handleRoomInfo(int count)
{
    this->setWindowTitle(
        QString("Landlords - 我是玩家 %1 / 房间人数 %2")
            .arg(m_state->playerId())
                .arg(count));
}

void GamePanel::handleStartGame()
{
    this->setWindowTitle(
        QString("Landlords - 我是玩家 %1 / 游戏开始")
            .arg(m_state->playerId()));
    if (m_endingPanel != nullptr)
    {
        m_endingPanel->close();
        m_endingPanel->deleteLater();
        m_endingPanel = nullptr;
    }

    clearAllPlayerDropCards();

    // 启动背景音乐
    m_bgm->startBGM(80);

    // 启动发牌动画
    startDispatchCard();
}

void GamePanel::handleGameOver(int winnerId)
{
    qDebug() << "handleGameOver, winnerId =" << winnerId;

        ui->btnGroup->hide();
        m_countDown->stopCountDown();
        m_bgm->stopBGM();

        m_contextMap[m_gameCtl->getLeftRobot()].isFrontSide = true;
        m_contextMap[m_gameCtl->getRightRobot()].isFrontSide = true;
        updatePlayerCards(m_gameCtl->getLeftRobot());
        updatePlayerCards(m_gameCtl->getRightRobot());

        updatePlayerScore();
        updatePlayerRoleImages();
        showEndingScorePanel();
}

void GamePanel::raiseOverlayWidgets()
{
    if (m_countDown != nullptr && m_countDown->isVisible())
       {
           m_countDown->raise();
       }

       if (ui->btnGroup != nullptr && ui->btnGroup->isVisible())
       {
           ui->btnGroup->raise();
       }

       if (m_animation != nullptr && m_animation->isVisible())
       {
           m_animation->raise();
       }

       if (m_endingPanel != nullptr && m_endingPanel->isVisible())
       {
           m_endingPanel->raise();
       }
}
