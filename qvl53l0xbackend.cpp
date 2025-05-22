#include "qvl53l0xbackend.h"

QVL53L0XBackend::QVL53L0XBackend(QSensor *sensor) : QSensorBackend(sensor)
{
    m_pollTimer = new QTimer(this);
    QObject::connect(m_pollTimer, SIGNAL(timeout()), this, SLOT(poll()));

    reportEvent("QVL53L0X BACKEND CREATED");

    setReading<QVL53L0XReading>(&m_reading);
    reading();

    addDataRate(1, 30);
}

QVL53L0XBackend::~QVL53L0XBackend()
{
    if(m_pollTimer)
    {
        if(m_pollTimer->isActive())
            m_pollTimer->stop();

        delete m_pollTimer;
    }

    QSensorBackend::~QSensorBackend();
}

void QVL53L0XBackend::start()
{
    if(m_pollTimer && m_pollTimer->isActive())
        return;

    if(!m_initialized && !initialize())
    {
        reportError("COULD NOT INITIALIZE SENSOR");
        handleFault();
        return;
    }

    m_pollTimer->setInterval(1000 / sensor()->dataRate());
    m_pollTimer->start();
}

void QVL53L0XBackend::stop()
{
    if(!m_pollTimer || !m_pollTimer->isActive())
        return;

    m_pollTimer->stop();
}

bool QVL53L0XBackend::isFeatureSupported(QSensor::Feature feature) const
{
    return false;
}

bool QVL53L0XBackend::initialize()
{
    QVL53L0X *sensor = qobject_cast<QVL53L0X*>(this->sensor());
    quint8 data = 0;

    if(!sensor)
        return false;

    m_bus = sensor->bus();
    m_address = sensor->address();

    QObject::connect(sensor, &QVL53L0X::busChanged, this, &QVL53L0XBackend::onSensorBusChanged);
    QObject::connect(sensor, &QVL53L0X::addressChanged, this, &QVL53L0XBackend::onSensorAddressChanged);
    QObject::connect(sensor, &QVL53L0X::dataRateChanged, this, &QVL53L0XBackend::onSesnorDataRateChanged);

    startI2C();

    if(!confirmChipID())
    {
        reportError("CHIP ID FAILED");
        return false;
    }

    //Set default I2C mode
    if(!writeRegisterByte(0x88, 0x00))
        return false;

    if(!writeRegisterByte(0x80, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x00))
        return false;

    if(!readRegisterByte(0x91, &m_stopByte))
        return false;

    if(!writeRegisterByte(0x00, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x80, 0x00))
        return false;

    // disable SIGNAL_RATE_MSRC (bit 1) and SIGNAL_RATE_PRE_RANGE (bit 4) limit checks

    if(!readRegisterByte((quint8)Register::MSRC_CONFIG_CONTROL, &data))
        return false;
    if(!writeRegisterByte((quint8)Register::MSRC_CONFIG_CONTROL, data | 0x12))
        return false;

    // set final range signal rate limit to 0.25 MCPS (million counts per second)
    setSignalRateLimit(0.25);

    if(!writeRegisterByte((quint8)Register::SYSTEM_SEQUENCE_CONFIG, 0xFF))
        return false;

    quint8 spadCount;
    bool spadIsAperture;

    if (!getSpadInfo(spadCount, spadIsAperture))
        return false;

    // The SPAD map (RefGoodSpadMap) is read by VL53L0X_get_info_from_device() in
    // the API, but the same data seems to be more easily readable from
    // GLOBAL_CONFIG_SPAD_ENABLES_REF_0 through _6, so read it from there
    uint8_t spadMap[6];
    if(!readRegisterData((quint8)Register::GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spadMap, 6))
        return false;

    // -- VL53L0X_set_reference_spads() begin (assume NVM values are valid)
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte((quint8)Register::DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00))
        return false;
    if(!writeRegisterByte((quint8)Register::DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte((quint8)Register::GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4))
        return false;

    uint8_t firstSpad = spadIsAperture ? 12 : 0; // 12 is the first aperture spad
    uint8_t spadsEnabled = 0;

    for (uint8_t i = 0; i < 48; i++)
    {
        if (i < firstSpad || spadsEnabled == spadCount)
        {
            // This bit is lower than the first one that should be enabled, or
            // (reference_spad_count) bits have already been enabled, so zero this bit
            spadMap[i / 8] &= ~(1 << (i % 8));
        }
        else if ((spadMap[i / 8] >> (i % 8)) & 0x1)
        {
            spadsEnabled++;
        }
    }

    if(!writeRegisterData((quint8)Register::GLOBAL_CONFIG_SPAD_ENABLES_REF_0, spadMap, 6))
        return false;

    // DefaultTuningSettings from vl53l0x_tuning.h
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x09, 0x00))
        return false;
    if(!writeRegisterByte(0x10, 0x00))
        return false;
    if(!writeRegisterByte(0x11, 0x00))
        return false;
    if(!writeRegisterByte(0x24, 0x01))
        return false;
    if(!writeRegisterByte(0x25, 0xFF))
        return false;
    if(!writeRegisterByte(0x75, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x4E, 0x2C))
        return false;
    if(!writeRegisterByte(0x48, 0x00))
        return false;
    if(!writeRegisterByte(0x30, 0x20))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x30, 0x09))
        return false;
    if(!writeRegisterByte(0x54, 0x00))
        return false;
    if(!writeRegisterByte(0x31, 0x04))
        return false;
    if(!writeRegisterByte(0x32, 0x03))
        return false;
    if(!writeRegisterByte(0x40, 0x83))
        return false;
    if(!writeRegisterByte(0x46, 0x25))
        return false;
    if(!writeRegisterByte(0x60, 0x00))
        return false;
    if(!writeRegisterByte(0x27, 0x00))
        return false;
    if(!writeRegisterByte(0x50, 0x06))
        return false;
    if(!writeRegisterByte(0x51, 0x00))
        return false;
    if(!writeRegisterByte(0x52, 0x96))
        return false;
    if(!writeRegisterByte(0x56, 0x08))
        return false;
    if(!writeRegisterByte(0x57, 0x30))
        return false;
    if(!writeRegisterByte(0x61, 0x00))
        return false;
    if(!writeRegisterByte(0x62, 0x00))
        return false;
    if(!writeRegisterByte(0x64, 0x00))
        return false;
    if(!writeRegisterByte(0x65, 0x00))
        return false;
    if(!writeRegisterByte(0x66, 0xA0))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x22, 0x32))
        return false;
    if(!writeRegisterByte(0x47, 0x14))
        return false;
    if(!writeRegisterByte(0x49, 0xFF))
        return false;
    if(!writeRegisterByte(0x4A, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x7A, 0x0A))
        return false;
    if(!writeRegisterByte(0x7B, 0x00))
        return false;
    if(!writeRegisterByte(0x78, 0x21))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x23, 0x34))
        return false;
    if(!writeRegisterByte(0x42, 0x00))
        return false;
    if(!writeRegisterByte(0x44, 0xFF))
        return false;
    if(!writeRegisterByte(0x45, 0x26))
        return false;
    if(!writeRegisterByte(0x46, 0x05))
        return false;
    if(!writeRegisterByte(0x40, 0x40))
        return false;
    if(!writeRegisterByte(0x0E, 0x06))
        return false;
    if(!writeRegisterByte(0x20, 0x1A))
        return false;
    if(!writeRegisterByte(0x43, 0x40))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x34, 0x03))
        return false;
    if(!writeRegisterByte(0x35, 0x44))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x31, 0x04))
        return false;
    if(!writeRegisterByte(0x4B, 0x09))
        return false;
    if(!writeRegisterByte(0x4C, 0x05))
        return false;
    if(!writeRegisterByte(0x4D, 0x04))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x44, 0x00))
        return false;
    if(!writeRegisterByte(0x45, 0x20))
        return false;
    if(!writeRegisterByte(0x47, 0x08))
        return false;
    if(!writeRegisterByte(0x48, 0x28))
        return false;
    if(!writeRegisterByte(0x67, 0x00))
        return false;
    if(!writeRegisterByte(0x70, 0x04))
        return false;
    if(!writeRegisterByte(0x71, 0x01))
        return false;
    if(!writeRegisterByte(0x72, 0xFE))
        return false;
    if(!writeRegisterByte(0x76, 0x00))
        return false;
    if(!writeRegisterByte(0x77, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x0D, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x80, 0x01))
        return false;
    if(!writeRegisterByte(0x01, 0xF8))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x8E, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x80, 0x00))
        return false;

    // -- VL53L0X_load_tuning_settings() end

    // "Set interrupt config to new sample ready"
    // -- VL53L0X_SetGpioConfig() begin

    if(!writeRegisterByte((quint8)Register::SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04))
        return false;

    if(!readRegisterByte((quint8)Register::GPIO_HV_MUX_ACTIVE_HIGH, &data))
        return false;

    if(!writeRegisterByte((quint8)Register::GPIO_HV_MUX_ACTIVE_HIGH, (data & ~0x10))) // active low
        return false;

    if(!writeRegisterByte((quint8)Register::SYSTEM_INTERRUPT_CLEAR, 0x01))
        return false;

    // -- VL53L0X_SetGpioConfig() end

    // "Disable MSRC and TCC by default"
    // MSRC = Minimum Signal Rate Check
    // TCC = Target CentreCheck
    // -- VL53L0X_SetSequenceStepEnable() begin
    if(!writeRegisterByte((quint8)Register::SYSTEM_SEQUENCE_CONFIG, 0xE8))
        return false;

    // -- VL53L0X_SetSequenceStepEnable() end

    // VL53L0X_StaticInit() end

    // VL53L0X_PerformRefCalibration() begin (VL53L0X_perform_ref_calibration())

    // -- VL53L0X_perform_vhv_calibration() begin
    if(!writeRegisterByte((quint8)Register::SYSTEM_SEQUENCE_CONFIG, 0x01))
        return false;

    if (!performSingleRefCalibration(0x40))
        return false;

    // -- VL53L0X_perform_vhv_calibration() end

    // -- VL53L0X_perform_phase_calibration() begin
    if(!writeRegisterByte((quint8)Register::SYSTEM_SEQUENCE_CONFIG, 0x02))
        return false;

    if (!performSingleRefCalibration(0x00))
        return false;

    // -- VL53L0X_perform_phase_calibration() end

    // "restore the previous Sequence Config"
    if(!writeRegisterByte((quint8)Register::SYSTEM_SEQUENCE_CONFIG, 0xE8))
        return false;

    // VL53L0X_PerformRefCalibration() end

    endI2C();

    m_initialized = true;

    return true;
}

bool QVL53L0XBackend::startI2C()
{
    if(m_i2c < 0)
    {
        if((m_i2c = open(m_bus.toStdString().c_str(), O_RDWR)) < 0)
        {
            reportError("COULD NOT OPEN I2C BUS");
            m_errno = errno;

            return false;
        }
    }

    if(ioctl(m_i2c, I2C_SLAVE, m_address) < 0)
    {
        reportError("DEVICE NOT FOUND");
        m_errno = errno;

        endI2C();

        return false;
    }

    reportEvent("I2C STARTED");

    return true;
}

bool QVL53L0XBackend::endI2C()
{
    if(m_i2c < 0)
        return true;

    if(close(m_i2c) < 0)
    {
        reportError("COULD NOT CLOSE I2C BUS");
        m_errno = errno;
        m_i2c = -1;
        return false;
    }

    m_i2c = -1;
    reportEvent("I2C ENDED");

    return true;
}

bool QVL53L0XBackend::readRegisterByte(quint8 reg, quint8 *data)
{
    quint8 *buffer = new quint8[1] { *data };

    struct i2c_msg messages[]
    {
        {
            .addr = m_address,
            .flags = 0,
            .len = 1,
            .buf = &reg
        },
        {
            .addr = m_address,
            .flags = I2C_M_RD,
            .len = 1,
            .buf = buffer
        }
    };

    struct i2c_rdwr_ioctl_data payload =
    {
        .msgs = messages,
        .nmsgs = 2
    };

    if(ioctl(m_i2c, I2C_RDWR, &payload) < 0)
    {
        reportError(QString("COULD NOT READ REGISTER %1").arg(reg, 2, 16, '0'));
        m_errno = errno;
        return false;
    }

    *data = buffer[0];
    reportEvent(QString("READ FROM REGISTER %1: %2").arg(reg, 2, 16, '0').arg(static_cast<quint8>(buffer[0]), 2, 16, '0'));

    return true;
}

bool QVL53L0XBackend::readRegisterWord(quint8 reg, quint16 *data)
{
    quint8 *buffer = new quint8[2];

    if(!readRegisterData(reg, buffer, 2))
        return false;

    *data = static_cast<quint16>((buffer[0] << 8) | buffer[1]);

    return true;
}

bool QVL53L0XBackend::readRegisterData(quint8 reg, quint8 *data, quint8 length)
{
    struct i2c_msg messages[]
    {
        {
            .addr = m_address,
            .flags = 0,
            .len = 1,
            .buf = &reg
        },
        {
            .addr = m_address,
            .flags = I2C_M_RD,
            .len = length,
            .buf = data
        }
    };

    struct i2c_rdwr_ioctl_data payload =
    {
        .msgs = messages,
        .nmsgs = 2
    };

    if(ioctl(m_i2c, I2C_RDWR, &payload) < 0)
    {
        reportError(QString("COULD NOT WRITE REGISTER %1").arg(reg, 2, 16, '0'));
        m_errno = errno;
        return false;
    }

    reportEvent(QString("READ FROM REGISTER %1").arg(reg, 2, 16, '0'));

    return true;
}

bool QVL53L0XBackend::writeRegisterByte(quint8 reg, quint8 data)
{
    quint8 *buffer = new quint8[2]
    {
        reg,
        data
    };

    struct i2c_msg messages[]
    {
        {
            .addr = m_address,
            .flags = 0,
            .len = 2,
            .buf = buffer
        }
    };

    struct i2c_rdwr_ioctl_data payload =
    {
        .msgs = messages,
        .nmsgs = 1
    };

    if(ioctl(m_i2c, I2C_RDWR, &payload) < 0)
    {
        delete [] buffer;
        reportError(QString("COULD NOT WRITE REGISTER %1").arg(reg, 2, 16, '0'));
        m_errno = errno;
        return false;
    }

    reportEvent(QString("WROTE TO REGISTER %1").arg(reg, 2, 16, '0'));

    delete [] buffer;
    return true;
}

bool QVL53L0XBackend::writeRegisterWord(quint8 reg, quint16 data)
{
    quint8 *buffer = new quint8[3]
    {
        reg,
        static_cast<quint8>((data >> 8) & 0xFF),
        static_cast<quint8>(data & 0xFF)
    };

    struct i2c_msg messages[]
    {
        {
            .addr = m_address,
            .flags = 0,
            .len = 3,
            .buf = buffer
        }
    };

    struct i2c_rdwr_ioctl_data payload =
    {
        .msgs = messages,
        .nmsgs = 1
    };

    if(ioctl(m_i2c, I2C_RDWR, &payload) < 0)
    {
        delete [] buffer;
        reportError(QString("COULD NOT WRITE REGISTER %1").arg(reg, 2, 16, '0'));
        m_errno = errno;
        return false;
    }

    delete [] buffer;

    reportEvent(QString("WROTE TO REGISTER %1").arg(reg, 2, 16, '0'));

    return true;
}

bool QVL53L0XBackend::writeRegisterData(quint8 reg, quint8 *buffer, quint16 length)
{
    quint8 *data = new quint8[length + 1];
    data[0] = reg;
    memcpy(&data[1], buffer, length);

    struct i2c_msg messages[]
    {
        {
            .addr = m_address,
            .flags = 0,
            .len = length + 1,
            .buf = data
        }
    };

    struct i2c_rdwr_ioctl_data payload =
    {
        .msgs = messages,
        .nmsgs = 1
    };

    if(ioctl(m_i2c, I2C_RDWR, &payload) < 0)
    {
        reportError(QString("COULD NOT WRITE REGISTER %1").arg(reg, 2, 16, '0'));
        m_errno = errno;
        return false;
    }

    reportEvent(QString("WROTE TO REGISTER %1").arg(reg, 2, 16, '0'));

    return true;
}

bool QVL53L0XBackend::confirmChipID()
{
    // quint8 data = 0;

    // if(!readRegisterByte((quint8)Register::IDENTIFICATION_MODEL_ID, &data))
    //     return false;

    // return data == m_chipId;

    return true;
}

bool QVL53L0XBackend::getSpadInfo(quint8 &count, bool &isAperture)
{
    quint8 tmp = 0;
    quint8 data = 0;

    if(!writeRegisterByte(0x80, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x06))
        return false;

    data = 0x00; //clear data for read
    if(!readRegisterByte(0x83, &data))
        return false;

    data |= 0x04;
    if(!writeRegisterByte(0x83, data))
        return false;

    if(!writeRegisterByte(0xFF, 0x07))
        return false;
    if(!writeRegisterByte(0x81, 0x01))
        return false;
    if(!writeRegisterByte(0x80, 0x01))
        return false;
    if(!writeRegisterByte(0x94, 0x6b))
        return false;
    if(!writeRegisterByte(0x83, 0x00))
        return false;

    qreal start = QDateTime::currentMSecsSinceEpoch();

    data = 0;
    while(data == 0x00)
    {
        //timeout
        if((QDateTime::currentMSecsSinceEpoch() - start) >= 25)
            return false;
        if(!readRegisterByte(0x83, &data))
            return false;
    }

    if(!writeRegisterByte(0x83, 0x01))
        return false;

    if(!readRegisterByte(0x92, &tmp))
        return false;

    count = tmp & 0x7f;
    isAperture = (tmp >> 7) & 0x01;

    if(!writeRegisterByte(0x81, 0x00))
        return false;
    if(!writeRegisterByte(0xFF, 0x06))
        return false;

    data = 0;
    if(!readRegisterByte(0x83, &data))
        return false;

    data &= ~0x04;
    if(!writeRegisterByte(0x83, data))
        return false;

    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x80, 0x00))
        return false;

    return true;
}

bool QVL53L0XBackend::readDistance()
{
    if(!writeRegisterByte(0x80, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x01))
        return false;
    if(!writeRegisterByte(0x00, 0x00))
        return false;
    if(!writeRegisterByte(0x91, m_stopByte))
        return false;
    if(!writeRegisterByte(0x00, 0x01))
        return false;
    if(!writeRegisterByte(0xFF, 0x00))
        return false;
    if(!writeRegisterByte(0x80, 0x00))
        return false;

    if(!writeRegisterByte((quint8)Register::SYSRANGE_START, 0x01))
        return false;

    // "Wait until start bit has been cleared"
    qreal start = QDateTime::currentMSecsSinceEpoch();
    quint8 data = 0x01;

    while (data & 0x01)
    {
        if ((QDateTime::currentMSecsSinceEpoch() - start) >= 25)
            return false;

        if(!readRegisterByte((quint8)Register::SYSRANGE_START, &data))
            return false;
    }

    data = 0;
    start = QDateTime::currentMSecsSinceEpoch();

    while ((data & 0x07) == 0)
    {
        if ((QDateTime::currentMSecsSinceEpoch() - start) >= 25)
            return false;

        if(!readRegisterByte((quint8)Register::RESULT_INTERRUPT_STATUS, &data))
            return false;
    }

    // assumptions: Linearity Corrective Gain is 1000 (default);
    // fractional ranging is not enabled
    quint16 range = 0;

    if(!readRegisterWord((quint8)Register::RESULT_RANGE_STATUS + 10, &range))
        return false;

    if(!writeRegisterByte((quint8)Register::SYSTEM_INTERRUPT_CLEAR, 0x01))
        return false;

    m_distance = range;

    return true;
}

void QVL53L0XBackend::poll()
{
    //start i2c
    if(!startI2C() || !readDistance() || !endI2C())
    {
        m_errno = errno; //errno is not set by endI2C()
        handleFault();
        return;
    }

    m_reading.setDistance(m_distance);
    newReadingAvailable();
}

void QVL53L0XBackend::handleFault()
{
    //TODO
}

bool QVL53L0XBackend::setSignalRateLimit(qreal limit)
{
    if ((limit < 0 ) || (limit > 511.99)) { return false; }

    // Q9.7 fixed point format (9 integer bits, 7 fractional bits)
    quint16 data = limit * (1 << 7);

    if(!writeRegisterWord((quint8)Register::FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, data))
        return false;

    return true;
}

void QVL53L0XBackend::reportEvent(QString message)
{
    //report event if backendDebug is true
    if(m_backendDebug)
        qDebug() << QString("** %1 - (QVL53L0X@%2:0x%3)").arg(message, m_bus).arg(m_address, 2, 16);
}

void QVL53L0XBackend::reportError(QString message)
{
    qDebug() << QString("!! ERROR: %1 - (QBMP280@%2:0x%3)").arg(message, m_bus).arg(m_address, 2, 16) << errno;
}

void QVL53L0XBackend::newLine()
{
    //add newline if backendDebug is true
    if(m_backendDebug)
        qDebug() << "";
}

void QVL53L0XBackend::onSensorBusChanged()
{
    QVL53L0X *sensor = qobject_cast<QVL53L0X*>(this->sensor());

    if(!sensor)
        return;

    m_bus = sensor->bus();
}

void QVL53L0XBackend::onSensorAddressChanged()
{
    QVL53L0X *sensor = qobject_cast<QVL53L0X*>(this->sensor());

    if(!sensor)
        return;

    m_address = sensor->address();
}

void QVL53L0XBackend::onSesnorDataRateChanged()
{
    stop();
    start();
}

// based on VL53L0X_perform_single_ref_calibration()
bool QVL53L0XBackend::performSingleRefCalibration(quint8 vhvInitByte)
{
    if(!writeRegisterByte((quint8)Register::SYSRANGE_START, 0x01 | vhvInitByte))
        return false;        // VL53L0X_REG_SYSRANGE_MODE_START_STOP

    qreal start = QDateTime::currentMSecsSinceEpoch();
    quint8 data = 0;

    while((data & 0x07) == 0)
    {
        if((QDateTime::currentMSecsSinceEpoch() - start) >= 25)
            return false;

        if(!readRegisterByte((quint8)Register::RESULT_INTERRUPT_STATUS, &data))
            return false;
    }

    if(!writeRegisterByte((quint8)Register::SYSRANGE_START, 0x01))
        return false;

    if(!writeRegisterByte((quint8)Register::SYSRANGE_START, 0x00))
        return false;

    return true;
}
