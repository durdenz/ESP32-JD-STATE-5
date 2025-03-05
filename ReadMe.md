# State Machine with GIF from SD Card and HW IRQ support

## Notes:

### ESP32 to TFT LCD Interface

| TFT LCD Touchscreen | ESP32 | COLOR |
| :--- | :--- | :--- |
| T_IRQ | GPIO 36 | BROWN |
| T_OUT | GPIO 39 | GRAY |
| T_DIN | GPIO 32 | YELLOW |
| T_CS | GPIO 33 | PURPLE |
| T_CLK | GPIO 25 | BLUE |
| SDO(MISO) | GPIO 12 | WHITE |
| LED | GPIO 21 | GREEN |
| SCK | GPIO 14 | YELLOW |
| SDI(MOSI) | GPIO 13 | BROWN |
| D/C | GPIO 2 | ORANGE |
| RESET | EN/RESET | ORANGE |
| CS  | GPIO 15 | GREEN |
| GND | GND | BLACK |
| VCC | 3V3 | RED |

| SD CARD READER | ESP32 | COLOR |
| :--- | :--- | :--- |
| SD_CS | GPIO 5 | BLUE |
| SD_MOSI | GPIO 13 | PURPLE |
| SD_MISO | GPIO 12 | GRAY |
| SD_SCK | GPIO 14 | WHITE |

| A4988 Motor Controller | ESP32 | COLOR |
| :--- | :--- | :--- |
| DIR (PIN 8) | GPIO 23 | YELLOW |
| STEP (PIN 7) | GPIO 22 | ORANGE |
| VDD (PIN 10) | 5V VIN | RED |
| GND (PIN 9) | GND | BLACK |
| VMOT (PIN 16) | +12V | RED |
| GND (PIN 15) | GND (12V) | BLACK |

| A4988 Motor Controller | NMEA 17 STEPPER MOTOR | COLOR |
| 1B (PIN 11) | 1B | BLACK |
| 1A (PIN 12)| 1A | GREEN |
| 2A (PIN 13)| 2A | BLUE |
| 2B (PIN 14) | 2B | RED |

## PreRequisites
- Connect ESP32 with TFT Display/Touchscreen and SD Card Reader per the wiring table above
- Place image files from SDCARD folder onto SD Card
- Ensure that the following Libraries are installed in the Arduino IDE:
    1. TFT_eSPI
    2. SD
    3. FS
    4. AnimatedGIF
    5. JPEGDecoder

## Build and Download
1. Copy ESP32_JD_STATE_5.ino into a new Sketch folder in Arduino IDE
2. Set Arduino IDE to ESP32 Dev Module and the correct USB port
3. Build and Upload Sketch to ESP32
4. (If necessary for debug) Open Serial Monitor and set Baud Rate to 9600 baud