/*
 * I2C Scanner for ESP32-C3 SuperMini
 * SDA = GPIO 0, SCL = GPIO 1
 */

#include <Wire.h>

void setup() {
    Serial.begin(115200);
    delay(1000);
    Wire.begin(0, 1);  // SDA=GPIO0, SCL=GPIO1
    Serial.println("\nI2C Scanner (SDA=GPIO0, SCL=GPIO1)\n");
}

void loop() {
    int found = 0;
    for (byte addr = 1; addr < 127; addr++) {
        Wire.beginTransmission(addr);
        if (Wire.endTransmission() == 0) {
            Serial.printf("  Found device at 0x%02X\n", addr);
            found++;
        }
    }

    if (found == 0)
        Serial.println("  No devices found — check wiring!");
    else
        Serial.printf("  %d device(s) found\n", found);

    Serial.println("  Rescanning in 3s...\n");
    delay(3000);
}
