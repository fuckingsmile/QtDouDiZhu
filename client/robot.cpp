#include "robot.h"

Robot::Robot(QObject *parent) : Player(parent)
{
    m_type = Player::Robot;
}

void Robot::prepareCallLord()
{
    // 网络版客户端：机器人只是“远端玩家占位对象”，不在本地思考/出牌
}

void Robot::preparePlayHand()
{
    // 网络版客户端：机器人只是“远端玩家占位对象”，不在本地思考/出牌
}

void Robot::thinkCallLord()
{
    // 网络版客户端：不在本地叫分
}

void Robot::thinkPlayHand()
{
    // 网络版客户端：不在本地出牌
}
