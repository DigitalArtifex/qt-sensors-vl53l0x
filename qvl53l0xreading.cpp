#include "qvl53l0xreading.h"
#include "qvl53l0x_p.h"

quint32 QVL53L0XReading::distance() const
{
    return d->distance;
}

void QVL53L0XReading::setDistance(quint32 distance)
{
    d->distance = distance;
}
