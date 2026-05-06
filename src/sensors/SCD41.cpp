#include <SensirionI2CScd4x.h>
#include <Wire.h>
#include <SCD41.h>

#ifdef NO_ERROR
#undef NO_ERROR
#endif
#define NO_ERROR 0

SensirionI2cScd4x sensor;
static char errorMessage[64];
static int16_t error;
static constexpr uint32_t SCD41_DATA_READY_TIMEOUT_MS = 10000;
static constexpr uint32_t SCD41_STOP_MEASUREMENT_DELAY_MS = 500;
static constexpr uint32_t SCD41_REINIT_DELAY_MS = 30;

static bool restartPeriodicMeasurement()
{
    error = sensor.stopPeriodicMeasurement();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return false;
    }

    delay(SCD41_STOP_MEASUREMENT_DELAY_MS);

    error = sensor.reinit();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute reinit(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return false;
    }

    delay(SCD41_REINIT_DELAY_MS);

    error = sensor.startPeriodicMeasurement();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return false;
    }

    Serial.println("SCD41 periodic measurement restarted");
    return true;
}

void PrintUint64(uint64_t &value)
{
    Serial.print("0x");
    Serial.print((uint32_t)(value >> 32), HEX);
    Serial.print((uint32_t)(value & 0xFFFFFFFF), HEX);
}

void initCOSensor()
{
    sensor.begin(Wire, SCD41_I2C_ADDR_62);

    uint64_t serialNumber = 0;
    delay(30);

    // Read out information about the sensor
    error = sensor.getSerialNumber(serialNumber);
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute getSerialNumber(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
    Serial.print("serial number: ");
    PrintUint64(serialNumber);
    Serial.println();

    error = sensor.startPeriodicMeasurement();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute startPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return;
    }
}

uint16_t readCO2()
{
    bool dataReady = false;
    uint16_t co2Concentration = 0;
    float temperature = 0.0;
    float relativeHumidity = 0.0;
    const uint32_t waitStart = millis();

    error = sensor.getDataReadyStatus(dataReady);
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute getDataReadyStatus(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return 0;
    }
    while (!dataReady)
    {
        if (millis() - waitStart >= SCD41_DATA_READY_TIMEOUT_MS)
        {
            Serial.println("SCD41 timeout waiting for dataReady, restarting sensor");
            restartPeriodicMeasurement();
            return 0;
        }

        Serial.println("Getting CO2 DATA");
        delay(100);
        error = sensor.getDataReadyStatus(dataReady);
        Serial.println(error);
        if (error != NO_ERROR)
        {
            Serial.print("Error trying to execute getDataReadyStatus(): ");
            errorToString(error, errorMessage, sizeof errorMessage);
            Serial.println(errorMessage);
            return 0;
        }
    }

    error =
        sensor.readMeasurement(co2Concentration, temperature, relativeHumidity);
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute readMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
        return 0;
    }

    return co2Concentration;
}
