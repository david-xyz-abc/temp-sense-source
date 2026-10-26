# Temperature sensor (DS18B20 + ESP32)

Live temperature dashboard served by an ESP32 over WiFi.

| Sketch | What it does |
|---|---|
| `ds18b20_test/` | Detects the DS18B20 and prints its ID and temperature on serial |
| `temp_web/` | Live temperature dashboard at `http://temperature.local` |

## Wiring

| DS18B20 wire | ESP32 pin |
|---|---|
| Red (power) | `3V3` |
| Black (ground) | `GND` |
| Yellow (data) | `D4` |

Plus a **4.7k resistor between `D4` and `3V3`** (pull-up for the data line).

## WiFi

Set your WiFi name and password at the top of `temp_web/temp_web.ino`
(replace `your-wifi-ssid` / `your-wifi-password`), or put the same two
`#define`s in a local `temp_web/secrets.h` (gitignored). 2.4 GHz networks only.

## Build / upload

Needs the ESP32 board package and the `OneWire` and `DallasTemperature` libraries.

```sh
arduino-cli lib install OneWire DallasTemperature
arduino-cli compile --fqbn esp32:esp32:esp32 temp_web
arduino-cli upload  --fqbn esp32:esp32:esp32 -p /dev/ttyUSB0 temp_web
```

Then open `http://temperature.local` (or the IP printed on serial at 115200 baud)
from a device on the same WiFi. On Android, use the IP address.
