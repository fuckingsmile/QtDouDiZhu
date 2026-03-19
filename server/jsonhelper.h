#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <QJsonObject>
#include <QJsonArray>
#include "card.h"
#include "cards.h"

QJsonObject cardToJson(const Card& card);
QJsonArray cardsToJson(Cards& cards);
Card jsonToCard(QJsonObject &obj);
Cards jsonToCards(QJsonArray &arr);
#endif // JSONHELPER_H
