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
    // Ensure sensor is in clean state
    error = sensor.wakeUp();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute wakeUp(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
    error = sensor.stopPeriodicMeasurement();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute stopPeriodicMeasurement(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
    error = sensor.reinit();
    if (error != NO_ERROR)
    {
        Serial.print("Error trying to execute reinit(): ");
        errorToString(error, errorMessage, sizeof errorMessage);
        Serial.println(errorMessage);
    }
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
        delay(100);
        error = sensor.getDataReadyStatus(dataReady);
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