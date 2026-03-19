#ifndef GAMECONTROL_H
#define GAMECONTROL_H

#include <QObject>
#include "robot.h"
#include "userplayer.h"

class GameControl : public QObject
{
    Q_OBJECT
public:
    explicit GameControl(QObject *parent = nullptr);

    // 网络版：仅用于创建三个座位玩家（UI 模型）
    void playerInit();

    Robot* getLeftRobot();
    Robot* getRightRobot();
    UserPlayer* getUserPlayer();

private:
    Robot* m_robotLeft = nullptr;
    Robot* m_robotRight = nullptr;
    UserPlayer* m_user = nullptr;
};

#endif // GAMECONTROL_H
