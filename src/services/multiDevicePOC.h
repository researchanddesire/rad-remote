#ifndef MULTI_DEVICE_POC_H
#define MULTI_DEVICE_POC_H

/**
 * Multi-Device Proof of Concept
 * 
 * This is a minimal test to verify the ESP32 can:
 * 1. Connect to 2 BLE devices simultaneously
 * 2. Control both devices at the same time
 * 
 * Usage:
 * 1. Power on 2 supported devices (OSSM and/or Lovense)
 * 2. Call startMultiDevicePOC() after BLE is initialized
 * 3. Watch serial output for connection status and control results
 * 
 * This bypasses the normal UI/state machine for testing purposes only.
 */

// Start the multi-device proof of concept test
void startMultiDevicePOC();

// Check if POC is currently running
bool isMultiDevicePOCRunning();

// Stop the POC and disconnect all devices
void stopMultiDevicePOC();

#endif // MULTI_DEVICE_POC_H
