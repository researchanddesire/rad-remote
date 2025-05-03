#ifndef PINS_H
#define PINS_H
#pragma once

namespace pins
{
    // LED strip
    constexpr uint8_t LED_PIN = 23;
    constexpr uint8_t NUM_LEDS = 3;

    // Screen pins
    constexpr uint8_t TFT_BACKLIGHT = 12;
    constexpr uint8_t TFT_RST = 13;
    constexpr uint8_t TFT_A0 = 14;
    constexpr uint8_t TFT_CS = 25;
    constexpr uint8_t TFT_SCK = 26;
    constexpr uint8_t TFT_SDA = 27;

    // Encoder pins
    constexpr uint8_t LEFT_ENCODER_A = 34;
    constexpr uint8_t LEFT_ENCODER_B = 35;
    constexpr uint8_t RIGHT_ENCODER_A = 18;
    constexpr uint8_t RIGHT_ENCODER_B = 19;

    // MCP23017 pin definitions
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

    // I2C pins
    constexpr uint8_t I2C_SDA = 21;
    constexpr uint8_t I2C_SCL = 22;

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
