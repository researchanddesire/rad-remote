#ifndef PINS_H
#define PINS_H
#pragma once

namespace pins
{
    // Screen pins (from Board Config.md)
    constexpr uint8_t TFT_CS = 4;   // Screen Cable Select
    constexpr uint8_t TFT_DC = 7;   // Screen A0
    constexpr uint8_t TFT_RST = 16; // Screen Reset
    constexpr uint8_t TFT_SCLK = 5; // Screen Clock
    constexpr uint8_t TFT_MOSI = 6; // Screen Data
    constexpr uint8_t TFT_BL = 15;  // Screen Backlight

    // LED strip
    constexpr uint8_t LED_PIN = 37;      // WS2812B LED pin
    constexpr uint8_t NUM_LEDS = 3;      // Number of LEDs
    constexpr uint8_t BUZZER_PIN = 2;    // Buzzer
    constexpr uint8_t VIBRATOR_PIN = 47; // Vibrator

    // I2C pins for battery fuel gauge
    constexpr uint8_t I2C_SDA = 8; // I2C SDA
    constexpr uint8_t I2C_SCL = 9; // I2C SCL

    // Button pins (from Board Config.md)
    constexpr uint8_t BTN_R_SHOULDER = 1;  // Right Shoulder Button
    constexpr uint8_t BTN_L_SHOULDER = 48; // Left Shoulder Button
    constexpr uint8_t BTN_L_ENC_A = 36;    // Left Encoder A
    constexpr uint8_t BTN_L_ENC_B = 35;    // Left Encoder B
    constexpr uint8_t BTN_R_ENC_A = 42;    // Right Encoder A
    constexpr uint8_t BTN_R_ENC_B = 41;    // Right Encoder B
    constexpr uint8_t BTN_UNDER_L = 38;    // Under Screen Left Button
    constexpr uint8_t BTN_UNDER_C = 39;    // Under Screen Centre Button
    constexpr uint8_t BTN_UNDER_R = 40;    // Under Screen Right Button

    // Encoder pins (from Board Config.md)
    constexpr uint8_t ENC_L_A = 36;
    constexpr uint8_t ENC_L_B = 35;
    constexpr uint8_t ENC_R_A = 42;
    constexpr uint8_t ENC_R_B = 41;

    // Legacy aliases for compatibility
    constexpr uint8_t LEFT_ENCODER_A = ENC_L_A;
    constexpr uint8_t LEFT_ENCODER_B = ENC_L_B;
    constexpr uint8_t RIGHT_ENCODER_A = ENC_R_A;
    constexpr uint8_t RIGHT_ENCODER_B = ENC_R_B;

    // MCP23017 pin definitions (if still used)
    constexpr uint8_t RIGHT_SHOULDER_BTN = 0;
    constexpr uint8_t BUZZER = 1;
    constexpr uint8_t GYRO_INT1 = 3;
    constexpr uint8_t GYRO_INT2 = 4;
    constexpr uint8_t LEFT_SHOULDER_BTN = 5;
    constexpr uint8_t VIBRATOR = 6;
    constexpr uint8_t REGULATOR_EN = 7;
    constexpr uint8_t FUEL_GAUGE = 8;
    constexpr uint8_t EXT_IO3 = 9;
    constexpr uint8_t EXT_IO4 = 10;
    constexpr uint8_t LEFT_BTN = 12;
    constexpr uint8_t CENTER_BTN = 13;
    constexpr uint8_t RIGHT_BTN = 14;

    // MCP23017 interrupt pin
    constexpr uint8_t MCP_INT_PIN = 16;

    // BQ27220 I2C address
    constexpr uint8_t BQ27220_ADDR = 0x55;

    // // Encoder pins
    // #define LEFT_ENCODER_A 34
    // #define LEFT_ENCODER_B 35
    // #define RIGHT_ENCODER_A 18
    // #define RIGHT_ENCODER_B 19

    // // MCP23017 pin definitions
    // #define RIGHT_SHOULDER_BTN 0
    // #define BUZZER 1
    // #define GYRO_INT1 3
    // #define GYRO_INT2 4
    // #define LEFT_SHOULDER_BTN 5
    // #define VIBRATOR 6
    // #define REGULATOR_EN 7
    // #define FUEL_GAUGE 8
    // #define EXT_IO3 9
    // #define EXT_IO4 10
    // #define LEFT_BTN 12
    // #define CENTER_BTN 13
    // #define RIGHT_BTN 14

}

#endif
