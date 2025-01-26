#include <Rfid134.h>

// Pin Definitions
const int resetPin = 4;     // RFID reset pin
const int motorPin1 = 5;    // DRV8833 Input 1 (PWM capable)
const int motorPin2 = 6;    // DRV8833 Input 2 (PWM capable)

// Configuration
const unsigned long long AUTHORIZED_TAG = 9120004103ULL;

// System States
bool isDoorOpen = false;
unsigned long lastTagDetectedTime = 0;
bool tagStillPresent = false;
const unsigned long TAG_TIMEOUT = 2000;  // Time in ms to wait before closing door
const unsigned long RFID_CHECK_INTERVAL = 1000; // Check RFID every 1 second
unsigned long lastRfidCheckTime = 0;

// Function declarations
void openDoor();
void closeDoor();
void stopMotor();

class RfidNotify {
public:
    static void OnError(Rfid134_Error errorCode) {
        Serial.print("Communication Error: ");
        Serial.println(errorCode);
    }

    static void OnPacketRead(const Rfid134Reading& reading) {
        Serial.print("Tag Detected: ");
        Serial.print(reading.country);
        Serial.print(" - ID: ");
        Serial.println(reading.id);

        if (reading.id == AUTHORIZED_TAG) {
            tagStillPresent = true; // Tag detected, update presence
            lastTagDetectedTime = millis();  // Update the last successful read time
            if (!isDoorOpen) {
                openDoor();
            }
        } else {
            Serial.println("Unauthorized tag detected");
        }
    }
};

// Instantiate the Rfid134 object using Hardware Serial
Rfid134<HardwareSerial, RfidNotify> rfid(Serial1);

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);
    rfid.begin();

    pinMode(resetPin, OUTPUT);
    pinMode(motorPin1, OUTPUT);
    pinMode(motorPin2, OUTPUT);

    digitalWrite(resetPin, HIGH);

    closeDoor();
    Serial.println("Smart Feeder Ready");
}

void loop() {
    unsigned long currentMillis = millis();

    // Check RFID antenna every second
    if (currentMillis - lastRfidCheckTime >= RFID_CHECK_INTERVAL) {
        lastRfidCheckTime = currentMillis;
        resetRfidModule();
        rfid.loop();  // Check for tags
    }

    // If no tag has been detected for a timeout period, close the door
    if (isDoorOpen && !tagStillPresent && (currentMillis - lastTagDetectedTime > TAG_TIMEOUT)) {
        Serial.println("Tag no longer present - closing door");
        closeDoor();
    }

    // Reset `tagStillPresent` for the next RFID check interval
    tagStillPresent = false;
}

void openDoor() {
    Serial.println("Opening door");
    analogWrite(motorPin1, 255); // Full speed forward
    analogWrite(motorPin2, 0);
    delay(1000);  // Adjust for door movement time
    stopMotor();
    isDoorOpen = true;
}

void closeDoor() {
    Serial.println("Closing door");
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 255); // Full speed reverse
    delay(1000);  // Adjust for door movement time
    stopMotor();
    isDoorOpen = false;
}

void stopMotor() {
    analogWrite(motorPin1, 0);
    analogWrite(motorPin2, 0);
}

void resetRfidModule() {
    Serial.println("Resetting RFID Module...");
    digitalWrite(resetPin, LOW);
    delay(100);
    digitalWrite(resetPin, HIGH);
}
