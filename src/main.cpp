#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NeoPixel.h>
#include <AiEsp32RotaryEncoder.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/timer.h>

// LED strip setup
#define LED_PIN 23
#define NUM_LEDS 3
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// PWM setup for buzzer
#define BUZZER_PWM_CHANNEL 0
#define BUZZER_PWM_FREQ 4000
#define BUZZER_PWM_RESOLUTION 8  // 8-bit resolution (0-255)
#define BUZZER_PWM_DUTY 128      // 50% duty cycle (128/255)
#define BUZZER_TIMER_GROUP TIMER_GROUP_0
#define BUZZER_TIMER_NUM TIMER_0

// Function declarations
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

void rainbow(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel((i+j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

// MCP23017 setup
Adafruit_MCP23X17 mcp;

// Encoder pins
#define LEFT_ENCODER_A 34
#define LEFT_ENCODER_B 35
#define RIGHT_ENCODER_A 18
#define RIGHT_ENCODER_B 19

// MCP23017 pin definitions
#define RIGHT_SHOULDER_BTN 0
#define BUZZER 1
#define GYRO_INT1 3
#define GYRO_INT2 4
#define LEFT_SHOULDER_BTN 5
#define VIBRATOR 6
#define REGULATOR_EN 7
#define FUEL_GAUGE 8
#define EXT_IO3 9
#define EXT_IO4 10
#define LEFT_BTN 12
#define CENTER_BTN 13
#define RIGHT_BTN 14

// Control variables
uint8_t brightness = 128;  // Initial brightness (0-255)
uint8_t rainbowSpeed = 20; // Initial speed (ms delay)
unsigned long vibratorStartTime = 0;
bool vibratorActive = false;
unsigned long lastDebugTime = 0;
bool buzzerActive = false;
unsigned long buzzerStartTime = 0;

// Rotary encoder instances
AiEsp32RotaryEncoder leftEncoder = AiEsp32RotaryEncoder(LEFT_ENCODER_A, LEFT_ENCODER_B, -1, -1);
AiEsp32RotaryEncoder rightEncoder = AiEsp32RotaryEncoder(RIGHT_ENCODER_A, RIGHT_ENCODER_B, -1, -1);

void IRAM_ATTR readLeftEncoder() {
    leftEncoder.readEncoder_ISR();
}

void IRAM_ATTR readRightEncoder() {
    rightEncoder.readEncoder_ISR();
}

// Button handling task
void buttonTask(void *parameter) {
    // Check if center button is actually pressed (debounce)
    if (mcp.digitalRead(CENTER_BTN) == LOW) {
        // Activate vibrator
        vibratorActive = true;
        vibratorStartTime = millis();
        mcp.digitalWrite(VIBRATOR, HIGH);
        
        // Activate buzzer
        buzzerActive = true;
        buzzerStartTime = millis();
        mcp.digitalWrite(BUZZER, HIGH);
    }
    vTaskDelete(NULL); // Delete this task when done
}

// MCP23017 interrupt handler - creates a task to handle the button press
void IRAM_ATTR handleMCPInterrupt() {
    TaskHandle_t buttonTaskHandle = NULL;
    xTaskCreatePinnedToCore(
        buttonTask,    // Task function
        "ButtonTask",  // Task name
        2048,          // Stack size
        NULL,          // Task parameters
        1,             // Task priority
        &buttonTaskHandle, // Task handle
        1              // Core to run on (core 1)
    );
}

// Timer ISR for buzzer PWM
void IRAM_ATTR buzzerTimerISR(void *arg) {
    if (buzzerActive) {
        mcp.digitalWrite(BUZZER, !mcp.digitalRead(BUZZER));
    }
}

void printDebugInfo() {
    Serial.println("\n=== IO Status ===");
    Serial.println("ESP32 Local IO:");
    Serial.printf("Left Encoder A: %d\n", digitalRead(LEFT_ENCODER_A));
    Serial.printf("Left Encoder B: %d\n", digitalRead(LEFT_ENCODER_B));
    Serial.printf("Right Encoder A: %d\n", digitalRead(RIGHT_ENCODER_A));
    Serial.printf("Right Encoder B: %d\n", digitalRead(RIGHT_ENCODER_B));
    Serial.printf("LED Pin: %d\n", digitalRead(LED_PIN));
    
    Serial.println("\nMCP23017 Remote IO:");
    Serial.printf("Right Shoulder Button: %d\n", mcp.digitalRead(RIGHT_SHOULDER_BTN));
    Serial.printf("Buzzer: %d\n", mcp.digitalRead(BUZZER));
    Serial.printf("Gyro INT1: %d\n", mcp.digitalRead(GYRO_INT1));
    Serial.printf("Gyro INT2: %d\n", mcp.digitalRead(GYRO_INT2));
    Serial.printf("Left Shoulder Button: %d\n", mcp.digitalRead(LEFT_SHOULDER_BTN));
    Serial.printf("Vibrator: %d\n", mcp.digitalRead(VIBRATOR));
    Serial.printf("Regulator Enable: %d\n", mcp.digitalRead(REGULATOR_EN));
    Serial.printf("Fuel Gauge: %d\n", mcp.digitalRead(FUEL_GAUGE));
    Serial.printf("Ext. IO3: %d\n", mcp.digitalRead(EXT_IO3));
    Serial.printf("Ext. IO4: %d\n", mcp.digitalRead(EXT_IO4));
    Serial.printf("Left Button: %d\n", mcp.digitalRead(LEFT_BTN));
    Serial.printf("Center Button: %d\n", mcp.digitalRead(CENTER_BTN));
    Serial.printf("Right Button: %d\n", mcp.digitalRead(RIGHT_BTN));
    
    Serial.println("\nControl Variables:");
    Serial.printf("Brightness: %d\n", brightness);
    Serial.printf("Rainbow Speed: %d\n", rainbowSpeed);
    Serial.printf("Vibrator Active: %s\n", vibratorActive ? "Yes" : "No");
    if (vibratorActive) {
        Serial.printf("Vibrator Time Remaining: %lu ms\n", 2000 - (millis() - vibratorStartTime));
    }
    Serial.printf("Buzzer Active: %s\n", buzzerActive ? "Yes" : "No");
    if (buzzerActive) {
        Serial.printf("Buzzer Time Remaining: %lu ms\n", 500 - (millis() - buzzerStartTime));
    }
    Serial.println("================\n");
}

void setup() {
  Serial.begin(115200);
  
  // Initialize I2C
  Wire.begin(21, 22); // SDA, SCL
  
  // Initialize MCP23017
  if (!mcp.begin_I2C()) {
    Serial.println("Error: MCP23017 not found!");
    while (1);
  }
  
  // Configure MCP23017 pins
  // Input pins with pull-up
  mcp.pinMode(RIGHT_SHOULDER_BTN, INPUT_PULLUP);
  mcp.pinMode(LEFT_SHOULDER_BTN, INPUT_PULLUP);
  mcp.pinMode(LEFT_BTN, INPUT_PULLUP);
  mcp.pinMode(CENTER_BTN, INPUT_PULLUP);
  mcp.pinMode(RIGHT_BTN, INPUT_PULLUP);
  mcp.pinMode(EXT_IO3, INPUT_PULLUP);
  mcp.pinMode(EXT_IO4, INPUT_PULLUP);
  mcp.pinMode(GYRO_INT1, INPUT_PULLUP);
  mcp.pinMode(GYRO_INT2, INPUT_PULLUP);
  
  // Output pins
  mcp.pinMode(BUZZER, OUTPUT);
  mcp.pinMode(VIBRATOR, OUTPUT);
  mcp.pinMode(REGULATOR_EN, OUTPUT);
  mcp.pinMode(FUEL_GAUGE, OUTPUT);

  // Configure MCP23017 interrupts
  mcp.setupInterrupts(true, false, LOW); // Enable interrupts, mirror INTA/B, active LOW
  mcp.setupInterruptPin(CENTER_BTN, CHANGE); // Interrupt on any change
  attachInterrupt(digitalPinToInterrupt(16), handleMCPInterrupt, FALLING); // INT A pin

  // Configure buzzer timer
  timer_config_t timerConfig = {
      .alarm_en = TIMER_ALARM_EN,
      .counter_en = TIMER_PAUSE,
      .intr_type = TIMER_INTR_LEVEL,
      .counter_dir = TIMER_COUNT_UP,
      .auto_reload = TIMER_AUTORELOAD_EN,
      .divider = 80  // 80MHz / 80 = 1MHz timer frequency
  };
  timer_init(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM, &timerConfig);
  timer_set_counter_value(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM, 0);
  timer_set_alarm_value(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM, 1000000 / (2 * BUZZER_PWM_FREQ)); // 50% duty cycle
  timer_enable_intr(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM);
  timer_isr_register(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM, buzzerTimerISR, NULL, ESP_INTR_FLAG_IRAM, NULL);

  // Initialize encoders
  leftEncoder.begin();
  rightEncoder.begin();
  
  leftEncoder.setup(readLeftEncoder);
  rightEncoder.setup(readRightEncoder);
  
  // Set encoder boundaries and step size
  leftEncoder.setBoundaries(0, 100, false); // 0-100% brightness
  leftEncoder.setAcceleration(0); // No acceleration for linear response
  rightEncoder.setBoundaries(1, 100, false); // 1-100ms delay
  rightEncoder.setAcceleration(0); // No acceleration for linear response
  
  // Set initial values
  leftEncoder.setEncoderValue(brightness * 100 / 255); // Convert 0-255 to 0-100
  rightEncoder.setEncoderValue(rainbowSpeed);
  
  // Initialize LED strip
  strip.begin();
  strip.setBrightness(brightness);
  strip.show(); // Initialize all pixels to 'off'
}

void handleEncoders() {
  // Update brightness from left encoder (0-100%)
  if (leftEncoder.encoderChanged()) {
    uint8_t percent = leftEncoder.readEncoder();
    brightness = (percent * 255) / 100; // Convert 0-100 to 0-255
    strip.setBrightness(brightness);
    strip.show();
  }
  
  // Update speed from right encoder (1-100ms)
  if (rightEncoder.encoderChanged()) {
    rainbowSpeed = rightEncoder.readEncoder();
  }
}

void handleVibrator() {
  if (vibratorActive && (millis() - vibratorStartTime >= 1000)) {
    vibratorActive = false;
    mcp.digitalWrite(VIBRATOR, LOW);
  }
}

void handleBuzzer() {
  if (buzzerActive) {
    if (millis() - buzzerStartTime >= 500) {
      buzzerActive = false;
      timer_pause(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM);
      mcp.digitalWrite(BUZZER, LOW);
    } else {
      timer_start(BUZZER_TIMER_GROUP, BUZZER_TIMER_NUM);
    }
  }
}

void loop() {
  handleEncoders();
  handleVibrator();
  handleBuzzer();
  
  // Show rainbow effect with current speed
  rainbow(rainbowSpeed);
  
  // Print debug info every 2 seconds
  if (millis() - lastDebugTime >= 2000) {
    printDebugInfo();
    lastDebugTime = millis();
  }
  
  delay(10); // Small delay to prevent overwhelming the system
}
