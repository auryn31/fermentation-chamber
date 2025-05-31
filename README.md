# Fermentation Chamber Controller

An ESP32-based intelligent fermentation chamber controller that maintains precise temperature and humidity control for optimal fermentation conditions. Features an OLED display, rotary encoder interface, and software PWM control for fan and heater management.

## 🎥 Demo Video

[![Fermentation Chamber Controller Demo](https://img.shields.io/badge/YouTube-Watch%20Demo-red?style=for-the-badge&logo=youtube)](https://youtu.be/tLW_WAPdoeg)

## Features

- **Precise Temperature Control**: Maintains target temperature using PID-like control with heater management
- **Humidity Monitoring**: Real-time humidity tracking with DHT11 sensor
- **Smart Fan Control**: Variable speed fan control based on temperature and humidity differentials
- **Interactive Interface**: OLED display with rotary encoder for easy parameter adjustment
- **Timer Functionality**: Built-in countdown timer for fermentation processes
- **Persistent Settings**: Automatically saves and restores user preferences
- **Software PWM**: Efficient software-based PWM control for both fan and heater
- **Voltage Compensation**: Automatic compensation for DHT11 sensor accuracy at different voltages

## Hardware Requirements

### Components
- Display: https://de.aliexpress.com/item/1005006514489341.html
- ESP32: https://de.aliexpress.com/item/1005007205044247.html
- Rotary Encoder: https://de.aliexpress.com/item/1005006551162496.html
- Heating Pad: https://de.aliexpress.com/item/1005007808067441.html
- Step Down: https://de.aliexpress.com/item/1005007092498838.html
- Fan: https://de.aliexpress.com/item/1005006306536871.html
- USB-C Power: https://de.aliexpress.com/item/1005006823353722.html
- MOSFET: https://de.aliexpress.com/item/1005006152077072.html
- DHT11: https://de.aliexpress.com/item/1005006144273755.html

### Libraries Used
- `U8g2` - OLED Display Library
- `DHT sensor library` - Temperature/Humidity Sensor
- `Ai Esp32 Rotary Encoder` - Rotary Encoder Interface

## Pin Configuration

| Component | ESP32-C3 Pin |
|-----------|--------------|
| DHT11 Data | GPIO 7 |
| Encoder CLK | GPIO 3 |
| Encoder DT | GPIO 2 |
| Encoder SW | GPIO 10 |
| Fan Control | GPIO 5 |
| Heater Control | GPIO 21 |
| I2C SDA | GPIO 8 |
| I2C SCL | GPIO 9 |

## Wiring Diagram

```
ESP32-C3 Mini
    │
    ├── GPIO 7  ──── DHT11 Data Pin
    ├── GPIO 3  ──── Rotary Encoder CLK
    ├── GPIO 2  ──── Rotary Encoder DT
    ├── GPIO 10 ──── Rotary Encoder SW (Button)
    ├── GPIO 8  ──── OLED SDA (I2C)
    ├── GPIO 9  ──── OLED SCL (I2C)
    ├── GPIO 5  ──── Fan Control (via MOSFET/Relay)
    └── GPIO 21 ──── Heater Control (via MOSFET/Relay)
```

## Installation

1. **Clone the Repository**
   ```bash
   git clone <repository-url>
   cd fermentation_chamber
   ```

2. **Install PlatformIO**
   - Install [PlatformIO IDE](https://platformio.org/platformio-ide) or PlatformIO Core
   - Alternatively, use the PlatformIO extension in VS Code

3. **Build and Upload**
   ```bash
   # Build the project
   pio build

   # Upload to ESP32
   pio upload

   # Monitor serial output (optional)
   pio device monitor
   ```

## Usage

### Basic Operation

1. **Power On**: The system will display current temperature and humidity readings
2. **Navigation**: 
   - Rotate encoder to navigate between menu items
   - Press encoder button to enter/exit edit mode
   - Long press (1 second) to access timer functions

3. **Menu Items**:
   - **Temperature Target**: Set desired temperature (0-40°C)
   - **Humidity Target**: Set desired humidity (0-100%)
   - **Timer**: Set countdown timer (0-999999 seconds)

### Control Logic

- **Heater**: Activates when temperature is below target minus threshold
- **Fan**: 
  - Primary function: cooling when temperature exceeds target
  - Secondary function: humidity control when temperature is within range
  - Minimum PWM ensures reliable fan operation

### Timer Functionality
- Set timer duration using rotary encoder
- Long press to start/stop timer
- Timer persists through power cycles
- Visual countdown display

## Configuration

Key parameters can be adjusted in `include/config.h`:

```cpp
// Control thresholds
#define TEMP_THRESHOLD_LOW 1      // Degrees below target to activate heater
#define FAN_PWM_MIN 38           // Minimum fan PWM (15% for reliable operation)
#define SENSOR_READ_INTERVAL 500  // Sensor reading frequency (ms)

// PWM frequency
#define FAN_PWM_FREQ_SOFT 10     // Software PWM frequency (Hz)
```

## Architecture

The project follows a functional programming approach with clear separation of concerns:

- **`main.cpp`**: Main loop and hardware initialization
- **`sensors.cpp`**: DHT11 sensor reading with voltage compensation
- **`controls.cpp`**: Fan and heater control logic
- **`display.cpp`**: OLED display management
- **`input.cpp`**: Rotary encoder and button handling
- **`timer.cpp`**: Timer functionality
- **`persistence.cpp`**: Settings storage and retrieval

## Troubleshooting

### Common Issues

1. **Sensor Reading Failures**
   - Check DHT11 wiring and power supply
   - Verify pin configuration in `config.h`

2. **Fan Not Starting**
   - Ensure `FAN_PWM_MIN` is set appropriately for your fan
   - Check MOSFET/relay wiring

3. **Display Issues**
   - Verify I2C connections (SDA/SCL)
   - Check I2C address in display initialization

4. **Encoder Not Responding**
   - Verify encoder wiring and pin configuration
   - Check for proper debouncing in code

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes following the coding standards:
   - Pure functions where possible
   - Minimal side effects
   - C++ best practices
   - Function comments in header files only
4. Submit a pull request

## License

This project is open source. Please check the license file for details.

## Acknowledgments

- Built with PlatformIO framework
- Uses Arduino framework for ESP32
- Inspired by the fermentation community's need for precise environmental control 