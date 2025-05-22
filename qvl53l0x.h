#ifndef QVL53L_X_H
#define QVL53L_X_H

#include <QObject>
#include <QSensor>
#include <QString>

#include "qvl53l0x_global.h"
#include "qvl53l0xreading.h"

QT_BEGIN_NAMESPACE

class QVL53L_X_EXPORT QVL53L0X : public QSensor
{
    Q_OBJECT
public:
    static inline char const * const sensorType = "QVL53L0X";

    explicit QVL53L0X(QObject *parent = nullptr);

    QVL53L0XReading *reading() const;

    QString bus() const;
    void setBus(const QString &bus);

    quint8 address() const;
    void setAddress(quint8 address);

signals:
    void busChanged();
    void addressChanged();

private:
    QString m_bus = "/dev/i2c-1"; //i2c bus path
    quint8 m_address = 0x52; //i2c device address

    Q_PROPERTY(QString bus READ bus WRITE setBus NOTIFY busChanged FINAL)
    Q_PROPERTY(quint8 address READ address WRITE setAddress NOTIFY addressChanged FINAL)
};

QT_END_NAMESPACE
#endif // QVL53L_X_H
