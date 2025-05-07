#include "encoder.h"

// Initialize the global service instances
AiEsp32RotaryEncoder leftEncoder(pins::LEFT_ENCODER_A, pins::LEFT_ENCODER_B, -1, -1);
AiEsp32RotaryEncoder rightEncoder(pins::RIGHT_ENCODER_A, pins::RIGHT_ENCODER_B, -1, -1);

void IRAM_ATTR readLeftEncoder()
{
    leftEncoder.readEncoder_ISR();
}

void IRAM_ATTR readRightEncoder()
{
    rightEncoder.readEncoder_ISR();
}

void initEncoderService()
{
    // Initialize encoders
    leftEncoder.begin();
    rightEncoder.begin();

    leftEncoder.setup(readLeftEncoder);
    rightEncoder.setup(readRightEncoder);

    // Set encoder boundaries and step size
    leftEncoder.setBoundaries(0, 100, false);  // 0-100% brightness
    leftEncoder.setAcceleration(0);            // No acceleration for linear response
    rightEncoder.setBoundaries(1, 100, false); // 1-100ms delay
    rightEncoder.setAcceleration(0);           // No acceleration for linear response

    // Set initial values
    leftEncoder.setEncoderValue(50);  // Start at 50%
    rightEncoder.setEncoderValue(50); // Start at 50ms
}