#ifndef LCD_STATUS_DISPLAY_H
#define LCD_STATUS_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

class LcdStatusDisplay {
private:
    static const uint8_t COLUMNS = 16;
    LiquidCrystal_I2C lcd;

    void printLine(uint8_t row, const String& text) {
        lcd.setCursor(0, row);
        for (uint8_t column = 0; column < COLUMNS; column++) {
            lcd.print(column < text.length() ? text.charAt(column) : ' ');
        }
    }

public:
    LcdStatusDisplay() : lcd(0x27, COLUMNS, 2) {}

    void begin() {
        Wire.begin(16, 18);
        lcd.init();
        lcd.backlight();
        show("Iniciando...", "Sistema reciclaje");
    }

    void show(const String& firstLine, const String& secondLine) {
        printLine(0, firstLine);
        printLine(1, secondLine);
    }
};

#endif
