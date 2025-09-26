#ifndef PINS_H
#define PINS_H
#pragma once

namespace pins {
    // Screen pins (from Board Config.md)
    constexpr uint8_t TFT_CS = 4;    // Screen Cable Select
    constexpr uint8_t TFT_DC = 7;    // Screen A0
    constexpr uint8_t TFT_RST = 16;  // Screen Reset
    constexpr uint8_t TFT_SCLK = 5;  // Screen Clock
    constexpr uint8_t TFT_MOSI = 6;  // Screen Data
    constexpr uint8_t TFT_BL = 15;   // Screen Backlight

// LED strip
#ifdef BOARD_HAS_PSRAM
    constexpr uint8_t LEFT_ENCODER_B = 10;
    constexpr uint8_t LEFT_ENCODER_A = 11;
    constexpr uint8_t LED_PIN = 12;  // WS2812B LED pin
#else
    constexpr uint8_t LEFT_ENCODER_B = 35;
    constexpr uint8_t LED_PIN = 37;  // WS2812B LED pin
    constexpr uint8_t LEFT_ENCODER_A = 36;
#endif
    constexpr uint8_t NUM_LEDS = 3;       // Number of LEDs
    constexpr uint8_t BUZZER_PIN = 2;     // Buzzer
    constexpr uint8_t VIBRATOR_PIN = 47;  // Vibrator

    // I2C pins for battery fuel gauge
    constexpr uint8_t I2C_SDA = 8;  // I2C SDA
    constexpr uint8_t I2C_SCL = 9;  // I2C SCL

    // Button pins (from Board Config.md)
    constexpr uint8_t BTN_R_SHOULDER = 1;   // Right Shoulder Button
    constexpr uint8_t BTN_L_SHOULDER = 48;  // Left Shoulder Button
    constexpr uint8_t BTN_UNDER_L = 38;     // Under Screen Left Button
    constexpr uint8_t BTN_UNDER_C = 39;     // Under Screen Centre Button
    constexpr uint8_t BTN_UNDER_R = 40;     // Under Screen Right Button

    // Legacy aliases for compatibility

    constexpr uint8_t RIGHT_ENCODER_A = 42;
    constexpr uint8_t RIGHT_ENCODER_B = 41;

    // MCP23017 pin definitions (if still used)
    constexpr uint8_t BUZZER = 1;
    constexpr uint8_t GYRO_INT1 = 3;
    constexpr uint8_t GYRO_INT2 = 4;
    constexpr uint8_t VIBRATOR = 6;
    constexpr uint8_t REGULATOR_EN = 7;
    constexpr uint8_t FUEL_GAUGE = 8;
    constexpr uint8_t EXT_IO3 = 9;
    constexpr uint8_t EXT_IO4 = 10;

    // MCP23017 interrupt pin
    constexpr uint8_t MCP_INT_PIN = 16;

    // BQ27220 I2C address
    constexpr uint8_t BQ27220_ADDR = 0x55;

}

#endif
