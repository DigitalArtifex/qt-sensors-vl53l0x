#ifndef QVL53L_XPLUGIN_H
#define QVL53L_XPLUGIN_H

#include <QObject>
#include <QSensorBackendFactory>
#include <QSensorPluginInterface>
#include <QSensorChangesInterface>

#include "qvl53l0x_global.h"
#include "qvl53l0xbackend.h"
#include "qvl53l0x.h"

QT_BEGIN_NAMESPACE

class QVL53L_X_EXPORT QVL53L0XPlugin : public QObject, public QSensorPluginInterface, public QSensorChangesInterface, public QSensorBackendFactory
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.qt-project.Qt.QSensorPluginInterface/1.0")
    Q_INTERFACES(QSensorPluginInterface QSensorChangesInterface)
public:
    void registerSensors() override
    {
        QSensor::defaultSensorForType(QVL53L0X::sensorType);
        QSensorManager::registerBackend(QVL53L0X::sensorType, QVL53L0XBackend::id, this);
    }

    void sensorsChanged() override
    {
        //register backend on initial load
        if(!QSensorManager::isBackendRegistered(QVL53L0X::sensorType, QVL53L0XBackend::id))
            QSensorManager::registerBackend(QVL53L0X::sensorType, QVL53L0XBackend::id, this);
    }

    QSensorBackend *createBackend(QSensor *sensor) override
    {
        if (sensor->identifier() == QVL53L0XBackend::id)
            return new QVL53L0XBackend(sensor);

        return 0;
    }
};

QT_END_NAMESPACE
#endif // QVL53L_XPLUGIN_H
