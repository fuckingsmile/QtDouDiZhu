#include "gamecontrol.h"
#include <QRandomGenerator>

GameControl::GameControl(QObject *parent) : QObject(parent)
{

}

void GameControl::playerInit()
{
    // 对象实例化
    m_robotLeft = new Robot("机器人A", this);
    m_robotRight = new Robot("机器人B", this);
    m_user = new UserPlayer("我自己", this);

    // 头像的显示
    m_robotLeft->setDirection(Player::Left);
    m_robotRight->setDirection(Player::Right);
    m_user->setDirection(Player::Right);

    // 性别
    Player::Sex sex;
    sex = (Player::Sex)QRandomGenerator::global()->bounded(2);
    m_robotLeft->setSex(sex);
    sex = (Player::Sex)QRandomGenerator::global()->bounded(2);
    m_robotRight->setSex(sex);
    sex = (Player::Sex)QRandomGenerator::global()->bounded(2);
    m_user->setSex(sex);

    // 出牌顺序
    // user
    m_user->setPrevPlayer(m_robotLeft);
    m_user->setNextPlayer(m_robotRight);

    // left robot
    m_robotLeft->setPrevPlayer(m_robotRight);
    m_robotLeft->setNextPlayer(m_user);

    // right robot
    m_robotRight->setPrevPlayer(m_user);
    m_robotRight->setNextPlayer(m_robotLeft);

}

Robot *GameControl::getLeftRobot()
{
    return m_robotLeft;
}

Robot *GameControl::getRightRobot()
{
    return m_robotRight;
}

UserPlayer *GameControl::getUserPlayer()
{
    return m_user;
}

