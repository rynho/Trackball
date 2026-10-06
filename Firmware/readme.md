Sensor:
- PAW3805 uses 3-wires SPI, need bit-banging when code in Arduino.
- ESP32-S2's default hardware SPI library expects two separate wires for data: MOSI (Master Out, Slave In) and MISO (Master In, Slave Out).
- The PAW3805 only has one data wire: SDIO. To talk to the sensor, ESP32-S2 must use the exact same pin to send commands (Output mode) and then quickly switch that pin to receive data (Input mode).
- Software SPI (Bit-Banging): Do not use the ESP32-S2's built-in SPI hardware engines at all. Instead, pick any 4 random digital pins (e.g., Pins 5, 6, 7, 8) and manually turn them HIGH and LOW using fast code (digitalWrite or direct register writes).
