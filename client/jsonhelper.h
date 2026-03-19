#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <QJsonObject>
#include <QJsonArray>
#include "card.h"
#include "cards.h"

Card jsonToCard(const QJsonObject& obj);
Cards jsonToCards(const QJsonArray& arr);
QJsonArray cardsToJson(Cards &cards);
QJsonObject cardToJson(const Card &card);
#endif // JSONHELPER_H
