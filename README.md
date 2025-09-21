# SimpleCalculator
A simple calculator using Arduino Uno, 4x4 Keypad and LCD display.

## Key project file:
**SimpleCalculator.ino**

## How to wire
- Connect your 4x4 keypad to the Arduino pins defined in the code (or adjust them if needed).  
- Connect the LCD I2C display to the **SDA**, **SCL**, **VCC**, and **GND** pins of the Arduino.  
- Make sure the I2C address in the code matches your display’s address (commonly `0x27`).
