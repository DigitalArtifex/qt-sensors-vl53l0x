#ifndef QVL53L_X_P_H
#define QVL53L_X_P_H

#include "qvl53l0x_global.h"

QT_BEGIN_NAMESPACE

class QVL53L_X_EXPORT QVL53L0XReadingPrivate
{
public:
    QVL53L0XReadingPrivate() : distance(0.0) { }

    qreal distance;
};

QT_END_NAMESPACE

#endif // QVL53L_X_P_H
