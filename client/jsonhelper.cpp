#include "jsonhelper.h"


QJsonObject cardToJson(const Card &card)
{
    QJsonObject obj;
    obj["point"] = static_cast<int>(card.point());
    obj["suit"]  = static_cast<int>(card.suit());
    return obj;
}

QJsonArray cardsToJson(Cards &cards)
{
    QJsonArray arr;

    CardList list = cards.toCardList();
    for (int i = 0; i < list.size(); ++i)
    {
        arr.append(cardToJson(list.at(i)));
    }

    return arr;
}


Card jsonToCard(const QJsonObject &obj)
{
    Card::CardPoint point = static_cast<Card::CardPoint>(obj["point"].toInt());
    Card::CardSuit suit = static_cast<Card::CardSuit>(obj["suit"].toInt());

    if (point == Card::Card_SJ || point == Card::Card_BJ) {
        suit = Card::Suit_Begin;
    }

    return Card(point, suit);
}

Cards jsonToCards(const QJsonArray &arr)
{
    Cards cards;
    for (int i = 0; i < arr.size(); ++i) {
        QJsonObject obj = arr[i].toObject();
        cards.add(jsonToCard(obj));
    }
    return cards;
}
