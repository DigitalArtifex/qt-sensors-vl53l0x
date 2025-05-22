#include "qvl53l0x.h"
#include "qvl53l0x_p.h"

IMPLEMENT_READING(QVL53L0XReading)

QVL53L0X::QVL53L0X(QObject *parent) : QSensor(sensorType, parent) {}

QVL53L0XReading *QVL53L0X::reading() const
{
    return qobject_cast<QVL53L0XReading*>(QSensor::reading());
}

QString QVL53L0X::bus() const
{
    return m_bus;
}

void QVL53L0X::setBus(const QString &bus)
{
    if (m_bus == bus)
        return;

    m_bus = bus;
    emit busChanged();
}

quint8 QVL53L0X::address() const
{
    return m_address;
}

void QVL53L0X::setAddress(quint8 address)
{
    if (m_address == address)
        return;

    m_address = address;
    emit addressChanged();
}
