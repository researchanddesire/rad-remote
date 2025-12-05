#include "encoder.h"

#include "memory.h"

// Initialize the global service instances
DRAM_ATTR AiEsp32RotaryEncoder leftEncoder(pins::LEFT_ENCODER_A,
                                           pins::LEFT_ENCODER_B, -1, -1, 4);
DRAM_ATTR AiEsp32RotaryEncoder rightEncoder(pins::RIGHT_ENCODER_A,
                                            pins::RIGHT_ENCODER_B, -1, -1, 4);

// Movement tracking state
static bool leftEncoderHasChanged = false;
static bool rightEncoderHasChanged = false;

void IRAM_ATTR readLeftEncoder() {
    leftEncoder.readEncoder_ISR();
    leftEncoderHasChanged = true;
}

void IRAM_ATTR readRightEncoder() {
    rightEncoder.readEncoder_ISR();
    rightEncoderHasChanged = true;
}

void initEncoderService() {
    // if the memory chip is found, then please reinit the encoder.
    // This is a wiring issue.
    if (isMemoryChipFound) {
        leftEncoder = AiEsp32RotaryEncoder(pins::LEFT_ENCODER_B,
                                           pins::LEFT_ENCODER_A, -1, -1, 4);
    }

    // Initialize encoders
    leftEncoder.begin();
    rightEncoder.begin();

    leftEncoder.setup(readLeftEncoder);
    rightEncoder.setup(readRightEncoder);

    // Set encoder boundaries and step size
    leftEncoder.setBoundaries(0, 100, false);  // 0-100% speed
    leftEncoder.setAcceleration(0);  // No acceleration for linear response
    rightEncoder.setBoundaries(
        0, 100,
        false);  // 0-100% focus switcher on "stroke", "sensation", "depth", etc
    rightEncoder.setAcceleration(0);  // No acceleration for linear response

    // Set initial values
    leftEncoder.setEncoderValue(50);   // Start at 50%
    rightEncoder.setEncoderValue(50);  // Start at 50ms
}

bool hasLeftEncoderChanged() { return hasLeftEncoderChanged(false); }
bool hasRightEncoderChanged() { return hasRightEncoderChanged(false); }

// Helper functions to check encoder change state
bool hasLeftEncoderChanged(bool reset) {
    bool changed = leftEncoderHasChanged;
    if (reset) {
        leftEncoderHasChanged = false;  // Reset after reading
    }
    return changed;
}

bool hasRightEncoderChanged(bool reset) {
    bool changed = rightEncoderHasChanged;
    if (reset) {
        rightEncoderHasChanged = false;  // Reset after reading
    }
    return changed;
}