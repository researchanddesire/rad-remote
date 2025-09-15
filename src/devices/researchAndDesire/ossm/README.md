# OSSM BLE Control Library - Basics

## Service & Device Info

- **Service UUID**: `522b443a-4f53-534d-0001-420badbabe69`
- **Device Name**: "OSSM"
- **Manufacturer**: "Research And Desire"

## Active Characteristics

### 1. Command Characteristic (Write)

- **UUID**: `522b443a-4f53-534d-0002-420badbabe69`
- **Purpose**: Send control commands

**Commands**:

```
set:speed:<0-100>        # Stroke speed percentage
set:stroke:<0-100>       # Stroke length percentage
set:depth:<0-100>        # Penetration depth percentage
set:sensation:<0-100>    # Sensation intensity percentage
set:pattern:<0-6>        # Stroke pattern (0-6)
go:simplePenetration     # Switch to simple penetration mode
go:strokeEngine          # Switch to stroke engine mode
go:menu                  # Return to main menu
```

**Responses**:

- Success: `ok:<original_command>`
- Failure: `fail:<original_command>`

### 2. State Characteristic (Read/Notify)

- **UUID**: `522b443a-4f53-534d-1000-420badbabe69`
- **Purpose**: Monitor device state and settings

**State JSON Format**:

```json
{
  "state": "<state_name>",
  "speed": <0-100>,
  "stroke": <0-100>,
  "sensation": <0-100>,
  "depth": <0-100>,
  "pattern": <0-6>
}
```

**Key States**:

- `idle` - Initializing
- `menu` - Main menu
- `simplePenetration` - Simple penetration mode
- `strokeEngine` - Stroke engine mode
- `error` - Error state

### 3. Speed Knob Config Characteristic (Write)

- **UUID**: `522b443a-4f53-534d-0010-420badbabe69`
- **Purpose**: Configure speed knob behavior

**Values**:

- `true` - Speed knob acts as upper limit for BLE commands (default)
- `false` - Speed knob and BLE speed are independent

### 4. Pattern List Characteristic (Read)

- **UUID**: `522b443a-4f53-534d-2000-420badbabe69`
- **Purpose**: Get available stroke patterns

**Pattern JSON Format**:

```json
[
  { "name": "Simple Stroke", "idx": 0 },
  { "name": "Teasing Pounding", "idx": 1 },
  { "name": "Robo Stroke", "idx": 2 },
  { "name": "Half'n'Half", "idx": 3 },
  { "name": "Deeper", "idx": 4 },
  { "name": "Stop'n'Go", "idx": 5 },
  { "name": "Insist", "idx": 6 }
]
```

## Connection Flow

1. Scan for "OSSM" device
2. Connect and discover service
3. Subscribe to state notifications
4. Read initial state and patterns
5. Send commands as needed
