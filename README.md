# Clear Zone ESP32 Firmware

Embedded software for the **Clear Zone** project, an indoor air quality monitoring system based on ESP32.

## Overview

This firmware collects air quality and environmental data, provisions Wi-Fi over BLE, registers the device through HTTPS, and publishes sensor readings to MQTT.

## Features

- ESP32 firmware built with PlatformIO and Arduino
- BLE-based onboarding for Wi-Fi setup
- Secure device registration over HTTPS
- MQTT telemetry publishing
- FreeRTOS task-based runtime
- Periodic averaged sensor readings

## Supported Sensors

- SHT31: temperature and humidity
- SCD41: CO2
- PMS5003: particulate matter (`PM1.0`, `PM2.5`, `PM10`)
- BME680: gas resistance and sensor humidity

## Build

```bash
pio run
```

## Flash

```bash
pio run -t upload
```

## Provisioning Flow

1. Power the device.
2. Hold the `BOOT` button for 5 seconds to enter pairing mode.
3. Connect over BLE and provide Wi-Fi credentials.
4. The device registers with the backend and starts publishing telemetry.

## MQTT Topics

The firmware publishes under:

```text
devices/<chip_id>/status
devices/<chip_id>/temp
devices/<chip_id>/co2
devices/<chip_id>/pms
devices/<chip_id>/bme
```
