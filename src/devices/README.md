### Devices Module

Technical documentation for developers adding new BLE devices to the remote.

## Supported Devices

- **Research And Desire**
  - **OSSM**: Supported
  - **LKBX**: Pending
  - **DTT**: Pending
- **Lovense**
  - **Domi 2**: Pending

## Registry Overview

Devices are discovered by their primary service UUID and instantiated via a factory registry.

- Registry map: `src/devices/registry.hpp`
- Known service UUIDs: `src/devices/serviceUUIDs.h`
- Device base class: `src/devices/device.hpp`

## How to Add a Device to the Registry

1. **Obtain the service UUID**

- Use a Bluetooth sniffing/inspection tool (e.g., Bluetility on macOS) to find the device's primary service UUID.

2. **Add the service UUID to `serviceUUIDs.h`**

- Store the UUID as a PROGMEM string constant.

```cpp
// src/devices/serviceUUIDs.h
static const char NEW_DEVICE_SERVICE_ID[] PROGMEM = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
```

3. **Register a factory in `registry.hpp`**

- Include your device header and add an entry mapping the service UUID to a factory that returns a `Device*` of your concrete class.

```cpp
// src/devices/registry.hpp
#include "your_vendor/your_device/your_device.h"

static const std::unordered_map<String, DeviceFactory> registry = {
    {OSSM_SERVICE_ID, []() -> Device* { return new OSSM(); }},
    {DOMI_SERVICE_ID, []() -> Device* { return new Domi2(); }},
    {NEW_DEVICE_SERVICE_ID, []() -> Device* { return new YourDeviceClass(); }}
};
```

4. **Provide a concrete `Device` implementation**

- Create a header/implementation pair for your device that derives from `Device` (see `src/devices/device.hpp`).  
- **Preferred file structure:**  
  Place your files in  
  `/src/devices/<brand_name>/<device_name>/<device>_device.h`  
  `/src/devices/<brand_name>/<device_name>/<device>_device.cpp`  
  For example, for a "FooBar" device from "Acme", use:  
  `/src/devices/acme/foobar/foobar_device.h`  
  `/src/devices/acme/foobar/foobar_device.cpp`

- Implement input handlers, drawing, and value sync methods as needed.
