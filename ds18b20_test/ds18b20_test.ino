// DS18B20 detection + temperature test for ESP32
// Wiring: red -> 3V3, black -> GND, yellow (data) -> D4
// Uses the ESP32's internal pull-up on the data line (a 4.7k resistor
// between data and 3V3 is more reliable if you have one)

#include <OneWire.h>
#include <DallasTemperature.h>

const int DATA_PIN = 4;

OneWire oneWire(DATA_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  pinMode(DATA_PIN, INPUT_PULLUP);
  sensors.begin();
}

void loop() {
  sensors.begin();  // re-scan so a sensor plugged in later is found
  int count = sensors.getDeviceCount();
  Serial.printf("Sensors found: %d\n", count);

  DeviceAddress addr;
  for (int i = 0; i < count; i++) {
    if (!sensors.getAddress(addr, i)) continue;
    sensors.requestTemperaturesByAddress(addr);
    float c = sensors.getTempC(addr);
    Serial.printf("  #%d  addr=", i);
    for (int b = 0; b < 8; b++) Serial.printf("%02X", addr[b]);
    if (c == DEVICE_DISCONNECTED_C) Serial.println("  read failed");
    else Serial.printf("  temp=%.2f C\n", c);
  }
  delay(1000);
}
