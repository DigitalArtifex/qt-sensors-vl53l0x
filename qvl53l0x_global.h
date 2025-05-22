#ifndef QVL53L_X_GLOBAL_H
#define QVL53L_X_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(QVL53L_X_LIBRARY)
#define QVL53L_X_EXPORT Q_DECL_EXPORT
#else
#define QVL53L_X_EXPORT Q_DECL_IMPORT
#endif

#endif // QT_SENSORS_VL53L_X_GLOBAL_H
