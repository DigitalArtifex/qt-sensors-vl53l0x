#ifndef QVL53L_XBACKEND_H
#define QVL53L_XBACKEND_H


#include <QObject>
#include <QString>
#include <QSensorBackend>
#include <QTimer>
#include <QThread>
#include <QDateTime>

#include "qvl53l0x_global.h"
#include "qvl53l0x.h"

#include "fcntl.h"
#include "i2c/smbus.h"
#include "linux/i2c-dev.h"
#include "linux/errno.h"
#include "sys/ioctl.h"

QT_BEGIN_NAMESPACE

class QVL53L_X_EXPORT QVL53L0XBackend : public QSensorBackend
{
    Q_OBJECT

    // register addresses from API vl53l0x_device.h (ordered as listed there)
    enum class Register : quint8
    {
        SYSRANGE_START                              = 0x00,

        SYSTEM_THRESH_HIGH                          = 0x0C,
        SYSTEM_THRESH_LOW                           = 0x0E,

        SYSTEM_SEQUENCE_CONFIG                      = 0x01,
        SYSTEM_RANGE_CONFIG                         = 0x09,
        SYSTEM_INTERMEASUREMENT_PERIOD              = 0x04,

        SYSTEM_INTERRUPT_CONFIG_GPIO                = 0x0A,

        GPIO_HV_MUX_ACTIVE_HIGH                     = 0x84,

        SYSTEM_INTERRUPT_CLEAR                      = 0x0B,

        RESULT_INTERRUPT_STATUS                     = 0x13,
        RESULT_RANGE_STATUS                         = 0x14,

        RESULT_CORE_AMBIENT_WINDOW_EVENTS_RTN       = 0xBC,
        RESULT_CORE_RANGING_TOTAL_EVENTS_RTN        = 0xC0,
        RESULT_CORE_AMBIENT_WINDOW_EVENTS_REF       = 0xD0,
        RESULT_CORE_RANGING_TOTAL_EVENTS_REF        = 0xD4,
        RESULT_PEAK_SIGNAL_RATE_REF                 = 0xB6,

        ALGO_PART_TO_PART_RANGE_OFFSET_MM           = 0x28,

        I2C_SLAVE_DEVICE_ADDRESS                    = 0x8A,

        MSRC_CONFIG_CONTROL                         = 0x60,

        PRE_RANGE_CONFIG_MIN_SNR                    = 0x27,
        PRE_RANGE_CONFIG_VALID_PHASE_LOW            = 0x56,
        PRE_RANGE_CONFIG_VALID_PHASE_HIGH           = 0x57,
        PRE_RANGE_MIN_COUNT_RATE_RTN_LIMIT          = 0x64,

        FINAL_RANGE_CONFIG_MIN_SNR                  = 0x67,
        FINAL_RANGE_CONFIG_VALID_PHASE_LOW          = 0x47,
        FINAL_RANGE_CONFIG_VALID_PHASE_HIGH         = 0x48,
        FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT = 0x44,

        PRE_RANGE_CONFIG_SIGMA_THRESH_HI            = 0x61,
        PRE_RANGE_CONFIG_SIGMA_THRESH_LO            = 0x62,

        PRE_RANGE_CONFIG_VCSEL_PERIOD               = 0x50,
        PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          = 0x51,
        PRE_RANGE_CONFIG_TIMEOUT_MACROP_LO          = 0x52,

        SYSTEM_HISTOGRAM_BIN                        = 0x81,
        HISTOGRAM_CONFIG_INITIAL_PHASE_SELECT       = 0x33,
        HISTOGRAM_CONFIG_READOUT_CTRL               = 0x55,

        FINAL_RANGE_CONFIG_VCSEL_PERIOD             = 0x70,
        FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        = 0x71,
        FINAL_RANGE_CONFIG_TIMEOUT_MACROP_LO        = 0x72,
        CROSSTALK_COMPENSATION_PEAK_RATE_MCPS       = 0x20,

        MSRC_CONFIG_TIMEOUT_MACROP                  = 0x46,

        SOFT_RESET_GO2_SOFT_RESET_N                 = 0xBF,
        IDENTIFICATION_MODEL_ID                     = 0xC0,
        IDENTIFICATION_REVISION_ID                  = 0xC2,

        OSC_CALIBRATE_VAL                           = 0xF8,

        GLOBAL_CONFIG_VCSEL_WIDTH                   = 0x32,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_0            = 0xB0,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_1            = 0xB1,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_2            = 0xB2,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_3            = 0xB3,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_4            = 0xB4,
        GLOBAL_CONFIG_SPAD_ENABLES_REF_5            = 0xB5,

        GLOBAL_CONFIG_REF_EN_START_SELECT           = 0xB6,
        DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         = 0x4E,
        DYNAMIC_SPAD_REF_EN_START_OFFSET            = 0x4F,
        POWER_MANAGEMENT_GO1_POWER_FORCE            = 0x80,

        VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           = 0x89,

        ALGO_PHASECAL_LIM                           = 0x30,
        ALGO_PHASECAL_CONFIG_TIMEOUT                = 0x30,
    };

public:
    static inline const char* id = "QVL53L0X-Backend";
    static inline const quint8 m_chipId = 0xEE; //i2c chip id

    explicit QVL53L0XBackend(QSensor *sensor = nullptr);
    ~QVL53L0XBackend();

    virtual void start() override;
    virtual void stop() override;
    virtual bool isFeatureSupported(QSensor::Feature feature) const override;

signals:

protected slots:
    void poll();

protected:
    bool initialize();
    bool startI2C();
    bool endI2C();
    bool readRegisterByte(quint8 reg, quint8 *data);
    bool readRegisterWord(quint8 reg, quint16 *data);
    bool readRegisterData(quint8 reg, quint8 *data, quint8 length);
    bool writeRegisterByte(quint8 reg, quint8 data);
    bool writeRegisterWord(quint8 reg, quint16 data);
    bool writeRegisterData(quint8 reg, quint8 *data, quint16 length);
    bool confirmChipID();
    bool getSpadInfo(quint8 &count, bool &isAperture);
    bool readDistance();
    void handleFault();
    bool setSignalRateLimit(qreal limit);
    void reportEvent(QString message);
    void reportError(QString message);
    void newLine();

    void onSensorBusChanged();
    void onSensorAddressChanged();
    void onSesnorDataRateChanged();

    bool performSingleRefCalibration(quint8 vhvInitByte);

private:
    int m_i2c = -1;
    int m_errno;
    QString m_bus;
    quint8 m_address;
    quint8 m_stopByte;

    bool m_initialized = false;
    bool m_backendDebug = true;
    QTimer *m_pollTimer = nullptr;

    quint32 m_distance = 0;
    QVL53L0XReading m_reading;
};

QT_END_NAMESPACE

#endif // QVL53L_XBACKEND_H
