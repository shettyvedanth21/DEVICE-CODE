#include "led_manager.h"
#include "config.h"
#include <Arduino.h>

static LedMode wifiMode = LedMode::OFF;
static LedMode mqttMode = LedMode::OFF;

static unsigned long wifiTimer = 0;
static unsigned long mqttTimer = 0;
static bool wifiState = false;
static bool mqttState = false;

static void tickPin(uint8_t pin, LedMode mode, unsigned long& timer,
                    bool& state, uint8_t otherPin = 255) {
    switch (mode) {
        case LedMode::OFF:
            digitalWrite(pin, LOW);
            break;
        case LedMode::SOLID_ON:
            digitalWrite(pin, HIGH);
            break;
        case LedMode::BLINK_SLOW:
        case LedMode::BLINK_FAST: {
            unsigned long period = (mode == LedMode::BLINK_SLOW) ? 1000UL : 150UL;
            if (millis() - timer >= period) {
                timer = millis();
                state = !state;
                digitalWrite(pin, state ? HIGH : LOW);
            }
            break;
        }
        case LedMode::AP_ALTERNATE:
            if (millis() - timer >= AP_LED_BLINK_MS) {
                timer = millis();
                state = !state;
                digitalWrite(pin,       state ? HIGH : LOW);
                if (otherPin != 255)
                    digitalWrite(otherPin, state ? LOW  : HIGH);
            }
            break;
    }
}

void LedManager::init() {
    pinMode(PIN_WIFI_LED, OUTPUT);
    pinMode(PIN_MQTT_LED, OUTPUT);
    digitalWrite(PIN_WIFI_LED, LOW);
    digitalWrite(PIN_MQTT_LED, LOW);
}

void LedManager::tick() {
    if (wifiMode == LedMode::AP_ALTERNATE) {
        tickPin(PIN_WIFI_LED, wifiMode, wifiTimer, wifiState, PIN_MQTT_LED);
    } else {
        tickPin(PIN_WIFI_LED, wifiMode, wifiTimer, wifiState);
        tickPin(PIN_MQTT_LED, mqttMode, mqttTimer, mqttState);
    }
}

void LedManager::setWifi(LedMode mode) { wifiMode = mode; }
void LedManager::setMqtt(LedMode mode) { mqttMode = mode; }
