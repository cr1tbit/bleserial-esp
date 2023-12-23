


#include <Arduino.h>
#include "BLESerial.h"

BLESerial bleSerial;

void setup() {
    Serial.begin(115200);

    // Create the BLE Device
    bleSerial.begin("Cyberbadge2.0");

}

void loop() {
    bleSerial.loop();
    if (bleSerial.available()) {
        Serial.println(bleSerial.readStringUntil('\0'));
    }
}