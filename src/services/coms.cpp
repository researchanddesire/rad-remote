/*
  OSSM Remote Communication Service
  Uses NimBLE to communicate with OSSM devices over BLE
*/

#include "coms.h"
#include <esp_log.h>
#include <NimBLEScan.h>

// Forward declarations
void onConnect(NimBLEClient *pClient);
void onDisconnect(NimBLEClient *pClient);
bool onScanResult(const NimBLEAdvertisedDevice *advertisedDevice);

// Client callbacks class
class ClientCallbacks : public NimBLEClientCallbacks
{
    void onConnect(NimBLEClient *pClient) override
    {
        ::onConnect(pClient);
    }

    void onDisconnect(NimBLEClient *pClient, int reason) override
    {
        ::onDisconnect(pClient);
        // Restart scanning after disconnect
        NimBLEDevice::getScan()->start(5, false, true);
    }
};

// Scan callbacks class
class ScanCallbacks : public NimBLEScanCallbacks
{
    void onResult(const NimBLEAdvertisedDevice *advertisedDevice) override
    {
        onScanResult(advertisedDevice);
    }

    void onScanEnd(const NimBLEScanResults &results, int reason) override
    {
        ESP_LOGD("COMS", "Scan ended, reason: %d, devices found: %d", reason, results.getCount());
        // Restart scanning if not connected
        if (!deviceConnected)
        {
            NimBLEDevice::getScan()->start(5, false, true);
        }
    }
};

// Global variables
NimBLEClient *pClient = nullptr;
bool deviceConnected = false;
bool serviceFound = false;
OSSMState currentOSSMState;

// BLE scan and connection callbacks
void onConnect(NimBLEClient *pClient)
{
    ESP_LOGI("COMS", "Connected to OSSM device");
    deviceConnected = true;

    // For now, assume services are available
    ESP_LOGI("COMS", "Connected, services will be discovered when needed");
    serviceFound = true;
}

void onDisconnect(NimBLEClient *pClient)
{
    ESP_LOGI("COMS", "Disconnected from OSSM device");
    deviceConnected = false;
    serviceFound = false;
}

bool onScanResult(const NimBLEAdvertisedDevice *advertisedDevice)
{
    ESP_LOGD("COMS", "Found device: %s", advertisedDevice->getName().c_str());

    if (advertisedDevice->getName() == OSSM_DEVICE_NAME)
    {
        ESP_LOGI("COMS", "Found OSSM device! Address: %s", advertisedDevice->getAddress().toString().c_str());

        // Stop scanning and connect
        NimBLEDevice::getScan()->stop();

        // Store the device for connection
        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(new ClientCallbacks());

        if (pClient->connect(advertisedDevice))
        {
            ESP_LOGI("COMS", "Successfully connected to OSSM");
        }
        else
        {
            ESP_LOGE("COMS", "Failed to connect to OSSM");
        }

        return true;
    }
    return false;
}

// Task to periodically read OSSM state
void ossmStateTask(void *parameter)
{
    while (true)
    {
        if (deviceConnected && serviceFound)
        {
            readOSSMState();
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Read every second
    }
}

void initBLE()
{
    ESP_LOGI("COMS", "Initializing BLE...");

    // Initialize BLE
    NimBLEDevice::init("OSSM-Remote");

    // Set scan parameters
    NimBLEScan *pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(new ScanCallbacks(), false);
    pScan->setActiveScan(true);
    pScan->setInterval(100);
    pScan->setWindow(99);

    ESP_LOGI("COMS", "BLE initialized, starting scan for OSSM devices...");

    // Start scanning for OSSM devices
    scanForOSSM();

    // Create task to periodically read OSSM state
    xTaskCreatePinnedToCore(
        ossmStateTask,
        "ossmStateTask",
        4 * configMINIMAL_STACK_SIZE,
        nullptr,
        1,
        nullptr,
        0);
}

void scanForOSSM()
{
    if (deviceConnected)
    {
        ESP_LOGD("COMS", "Already connected to OSSM device");
        return;
    }

    ESP_LOGI("COMS", "Scanning for OSSM devices...");
    NimBLEDevice::getScan()->start(5, false, true); // Scan for 5 seconds, restart when done
}

void connectToOSSM()
{
    if (deviceConnected)
    {
        ESP_LOGD("COMS", "Already connected to OSSM device");
        return;
    }

    // If we have a stored device, try to connect
    if (pClient && !pClient->isConnected())
    {
        ESP_LOGI("COMS", "Attempting to reconnect to OSSM...");
        pClient->connect();
    }
    else
    {
        // Start scanning again
        NimBLEDevice::getScan()->start(5, false, true);
    }
}

void disconnectFromOSSM()
{
    if (pClient && pClient->isConnected())
    {
        pClient->disconnect();
    }
}

void sendCommand(const String &command)
{
    if (!deviceConnected || !serviceFound)
    {
        ESP_LOGW("COMS", "Cannot send command - not connected to OSSM");
        return;
    }

    ESP_LOGD("COMS", "Sending command: %s", command.c_str());

    // Get the service and characteristic
    NimBLERemoteService *pService = pClient->getService(OSSM_SERVICE_UUID);
    if (pService == nullptr)
    {
        ESP_LOGE("COMS", "Service not found");
        return;
    }

    NimBLERemoteCharacteristic *pCharacteristic = pService->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
    if (pCharacteristic == nullptr)
    {
        ESP_LOGE("COMS", "Characteristic not found");
        return;
    }

    // Send the command
    if (pCharacteristic->canWrite())
    {
        pCharacteristic->writeValue(command.c_str());
        ESP_LOGD("COMS", "Command sent successfully");
    }
    else
    {
        ESP_LOGE("COMS", "Characteristic is not writable");
    }
}

void sendSettings(SettingPercents settings)
{
    if (!deviceConnected || !serviceFound)
    {
        ESP_LOGW("COMS", "Cannot send settings - not connected to OSSM");
        return;
    }

    // Create JSON command for settings
    DynamicJsonDocument doc(200);
    doc["command"] = "set";
    doc["speed"] = static_cast<int>(settings.speed);
    doc["stroke"] = static_cast<int>(settings.stroke);
    doc["sensation"] = static_cast<int>(settings.sensation);
    doc["depth"] = static_cast<int>(settings.depth);
    doc["pattern"] = static_cast<int>(settings.pattern);

    String jsonCommand;
    serializeJson(doc, jsonCommand);

    ESP_LOGD("COMS", "Sending settings: %s", jsonCommand.c_str());
    sendCommand(jsonCommand);
}

void readOSSMState()
{
    if (!deviceConnected || !serviceFound)
    {
        ESP_LOGW("COMS", "Cannot read state - not connected to OSSM");
        return;
    }

    // Get the service and characteristic
    NimBLERemoteService *pService = pClient->getService(OSSM_SERVICE_UUID);
    if (pService == nullptr)
    {
        ESP_LOGE("COMS", "Service not found");
        return;
    }

    NimBLERemoteCharacteristic *pCharacteristic = pService->getCharacteristic(OSSM_CHARACTERISTIC_UUID);
    if (pCharacteristic == nullptr)
    {
        ESP_LOGE("COMS", "Characteristic not found");
        return;
    }

    // Read the characteristic value
    if (pCharacteristic->canRead())
    {
        std::string value = pCharacteristic->readValue();
        ESP_LOGD("COMS", "Read OSSM state: %s", value.c_str());

        // Parse JSON response
        DynamicJsonDocument doc(200);
        DeserializationError error = deserializeJson(doc, value);

        if (!error)
        {
            currentOSSMState.state = doc["state"].as<String>();
            currentOSSMState.speed = doc["speed"].as<uint8_t>();
            currentOSSMState.stroke = doc["stroke"].as<uint8_t>();
            currentOSSMState.sensation = doc["sensation"].as<uint8_t>();
            currentOSSMState.depth = doc["depth"].as<uint8_t>();
            currentOSSMState.pattern = doc["pattern"].as<uint8_t>();

            ESP_LOGD("COMS", "Parsed state: %s, speed: %d, stroke: %d, sensation: %d, depth: %d, pattern: %d",
                     currentOSSMState.state.c_str(), currentOSSMState.speed, currentOSSMState.stroke,
                     currentOSSMState.sensation, currentOSSMState.depth, currentOSSMState.pattern);
        }
        else
        {
            ESP_LOGE("COMS", "Failed to parse JSON: %s", error.c_str());
        }
    }
    else
    {
        ESP_LOGE("COMS", "Characteristic is not readable");
    }
}

// Legacy function for backward compatibility
void sendESPNow(SettingPercents newSettings)
{
    sendSettings(newSettings);
}

// Mode control commands
void goToMenu()
{
    ESP_LOGI("COMS", "Sending go:menu command");
    sendCommand("go:menu");
}

void goToSimplePenetration()
{
    ESP_LOGI("COMS", "Sending go:simplePenetration command");
    sendCommand("go:simplePenetration");
}

void goToStrokeEngine()
{
    ESP_LOGI("COMS", "Sending go:strokeEngine command");
    sendCommand("go:strokeEngine");
}

// Parameter setting commands
void setStroke(uint8_t value)
{
    if (value > 100)
        value = 100;
    String command = "set:stroke:" + String(value);
    ESP_LOGI("COMS", "Sending %s command", command.c_str());
    sendCommand(command);
}

void setDepth(uint8_t value)
{
    if (value > 100)
        value = 100;
    String command = "set:depth:" + String(value);
    ESP_LOGI("COMS", "Sending %s command", command.c_str());
    sendCommand(command);
}

void setSensation(uint8_t value)
{
    if (value > 100)
        value = 100;
    String command = "set:sensation:" + String(value);
    ESP_LOGI("COMS", "Sending %s command", command.c_str());
    sendCommand(command);
}

void setSpeed(uint8_t value)
{
    if (value > 100)
        value = 100;
    String command = "set:speed:" + String(value);
    ESP_LOGI("COMS", "Sending %s command", command.c_str());
    sendCommand(command);
}

void setPattern(uint8_t value)
{
    if (value > 6)
        value = 6;
    String command = "set:pattern:" + String(value);
    ESP_LOGI("COMS", "Sending %s command", command.c_str());
    sendCommand(command);
}

// Utility function to get current OSSM state
auto getOSSMState()
{
    return currentOSSMState;
}

// Utility function to check if connected
bool isConnectedToOSSM()
{
    return deviceConnected && serviceFound;
}