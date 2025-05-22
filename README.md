# qt-sensors-vl53l0x
Baremetal I2C QtSensor Plugin for VL53L0X ToF Sensor
# Installation

Run the following commands in a terminal session to download and setup the plugin
```console
git clone https://github.com/DigitalArtifex/qt-sensors-mpu6050.git
mkdir qt-sensors-mpu6050/build
cd qt-sensors-mpu6050/build
```

Next, we will need to configure the project against your Qt Installation by executing `qt-cmake` located in `<Qt Path>/<Version>/<Arch>/bin`. Typically, the Qt Path will be `~/Qt`, but can be `/opt/Qt` if installed with sudo, or elsewhere if configured differently. The example assumes Qt Path to be `/opt/Qt`, Qt Version to be `6.9.0` and the arch to be `gcc_arm64`

```
/opt/Qt/6.9.0/gcc_arm64/bin/qt-cmake -S ../ -B ./
```

Once configured, we will switch to the system provided cmake. If Qt is installed to `~/` there is no need to execute `cmake install` with sudo

```
cmake --build ./
sudo cmake --install ./
```

# Usage

## Adding the reference (CMake)

```cmake
target_link_libraries(
  MyApp
  Qt${QT_VERSION_MAJOR}::Core
  Qt${QT_VERSION_MAJOR}::Sensors
  i2c
  qt${QT_VERSION_MAJOR}-sensors-vl53l0x
)
```

## Using in code

```cpp
#include <QCoreApplication>
#include <QVL53L0X/qvl53l0x.h>

QVL53L0X *vl53l0x;

void reading()
{
    QVL53L0XReading *reading = qobject_cast<QVL53L0XReading*>(vl53l0x->reading());

    if(reading)
        qDebug() << reading->distance() << "mm"; // readings are only accurate up to 2m
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // Set up code that uses the Qt event loop here.
    // Call a.quit() or a.exit() to quit the application.
    // A not very useful example would be including
    // #include <QTimer>
    // near the top of the file and calling
    // QTimer::singleShot(5000, &a, &QCoreApplication::quit);
    // which quits the application after 5 seconds.

    // If you do not need a running Qt event loop, remove the call
    // to a.exec() or use the Non-Qt Plain C++ Application template.

    vl53l0x = new QVL53L0X;
    vl53l0x->connectToBackend();
    vl53l0x->setDataRate(1);
    vl53l0x->setAddress(0x29); //DIY More

    QObject::connect(vl53l0x, &QVL53L0X::readingChanged, &reading);
    vl53l0x->start();

    return a.exec();
}
```
