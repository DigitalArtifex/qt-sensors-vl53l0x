#ifndef QVL53L_XREADING_H
#define QVL53L_XREADING_H

#include <QSensor>
#include <QSensorReading>

#include "qvl53l0x_global.h"

QT_BEGIN_NAMESPACE

class QVL53L0XReadingPrivate;

class QVL53L_X_EXPORT QVL53L0XReading : public QSensorReading
{
    Q_OBJECT
    Q_PROPERTY(quint32 distance READ distance)
    DECLARE_READING(QVL53L0XReading)
public:
    quint32 distance() const;
    void setDistance(quint32 distance);
};

class QVL53L_X_EXPORT QVL53L0XFilter : public QSensorFilter
{
public:
    virtual bool filter(QVL53L0XReading *reading) = 0;
private:
    bool filter(QSensorReading *reading) override { return filter(static_cast<QVL53L0XReading*>(reading)); }
};

QT_END_NAMESPACE

#endif // QVL53L_XREADING_H
