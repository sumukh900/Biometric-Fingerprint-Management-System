#include <Adafruit_Fingerprint.h>
#include <Servo.h>

#if (defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(2, 3);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);
Servo myServo;  
const int servoPin = 9;  

uint8_t id;

void setup()
{
  Serial.begin(9600);
  while (!Serial);  
  delay(100);
  Serial.println("\n\nFingerprint Sensor Management");

  // Initialize fingerprint sensor
  finger.begin(57600);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
    while (1) { delay(1); }
  }

  myServo.attach(servoPin);  // Attaches the servo on pin 9 to the servo object
  myServo.write(0);  // Initialize servo position
}

void loop()
{
  Serial.println("Select mode: (e)nroll, (v)erify, (d)elete");
  
  while (!Serial.available());
  char mode = Serial.read();
  
  // Clear any remaining characters in the buffer (like newline)
  while (Serial.available()) {
    Serial.read();
  }
  
  switch (tolower(mode)) {
    case 'e':
      enrollFingerprint();
      break;
    case 'v':
      verifyFingerprint();
      break;
    case 'd':
      deleteFingerprint();
      break;
    default:
      Serial.println("Invalid mode! Please select (e), (v), or (d).");
  }
}

void enrollFingerprint() {
  Serial.println("Enrolling fingerprint...");
  Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as...");
  id = readnumber();
  if (id == 0) {
     return;
  }
  Serial.print("Enrolling ID #");
  Serial.println(id);

  while (!getFingerprintEnroll() );
}

uint8_t getFingerprintEnroll() {
  int p = -1;
  Serial.print("Waiting for valid finger to enroll as #"); Serial.println(id);
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error in converting image");
    return p;
  }

  Serial.println("Remove finger");
  delay(2000);
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  Serial.print("ID "); Serial.println(id);
  Serial.println("Place same finger again");
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    switch (p) {
      case FINGERPRINT_OK:
        Serial.println("Image taken");
        break;
      case FINGERPRINT_NOFINGER:
        Serial.println("No finger detected");
        break;
      case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        break;
      case FINGERPRINT_IMAGEFAIL:
        Serial.println("Imaging error");
        break;
      default:
        Serial.println("Unknown error");
        break;
    }
  }

  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Error in converting image");
    return p;
  }

  p = finger.createModel();
  if (p == FINGERPRINT_OK) {
    Serial.println("Prints matched!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_ENROLLMISMATCH) {
    Serial.println("Fingerprints did not match");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Stored!");
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_BADLOCATION) {
    Serial.println("Could not store in that location");
    return p;
  } else if (p == FINGERPRINT_FLASHERR) {
    Serial.println("Error writing to flash");
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  return true;
}

void verifyFingerprint() {
  Serial.println("Verifying fingerprint...");
  Serial.println("Place your finger on the sensor");
  
  while (true) {  // Continuous loop
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
      switch (p) {
        case FINGERPRINT_NOFINGER:
          Serial.println("Waiting for finger...");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
      delay(1000);  // Wait a second before trying again
      continue;  // Skip the rest of the loop and start over
    }

    // If we get here, we've successfully captured an image
    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
      Serial.println("Failed to convert image.");
      continue;
    }

    p = finger.fingerFastSearch();
    if (p == FINGERPRINT_OK) {
      Serial.print("Found ID #"); Serial.print(finger.fingerID);
      Serial.print(" with confidence of "); Serial.println(finger.confidence);
      
      // Fingerprint verified, activate servo
      Serial.println("Fingerprint verified! Activating servo.");
      myServo.write(90);  // Turn servo to 90 degrees
      delay(1000);  // Keep servo activated for 1 second
      myServo.write(0);  // Return servo to initial position
    } else if (p == FINGERPRINT_NOTFOUND) {
      Serial.println("Fingerprint not recognized.");
    } else {
      Serial.println("Error in fingerprint search.");
    }
    Serial.println("Verify another? (y/n)");
    while (!Serial.available());
    char response = Serial.read();
    if (response != 'y' && response != 'Y') {
      break;  // Exit the verification loop
    }
    Serial.println("Place your finger on the sensor");
  }

  Serial.println("Exiting verification mode.");
}

void deleteFingerprint() {
  Serial.println("Delete Fingerprint");
  Serial.println("Place finger to delete on the sensor");

  while (true) {
    uint8_t p = finger.getImage();
    if (p != FINGERPRINT_OK) {
      switch (p) {
        case FINGERPRINT_NOFINGER:
          Serial.println("Waiting for finger...");
          break;
        case FINGERPRINT_PACKETRECIEVEERR:
          Serial.println("Communication error");
          break;
        case FINGERPRINT_IMAGEFAIL:
          Serial.println("Imaging error");
          break;
        default:
          Serial.println("Unknown error");
          break;
      }
      delay(1000);
      continue;
    }

    p = finger.image2Tz();
    if (p != FINGERPRINT_OK) {
      Serial.println("Failed to convert image");
      return;
    }

    p = finger.fingerFastSearch();
    if (p != FINGERPRINT_OK) {
      Serial.println("Finger not found in database.");
      return;
    }

    // Found a match!
    Serial.print("Found ID #"); Serial.print(finger.fingerID);
    Serial.print(" with confidence of "); Serial.println(finger.confidence);

    Serial.print("Deleting ID #"); Serial.println(finger.fingerID);

    p = finger.deleteModel(finger.fingerID);

    if (p == FINGERPRINT_OK) {
      Serial.println("Deleted!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
      Serial.println("Communication error");
    } else if (p == FINGERPRINT_BADLOCATION) {
      Serial.println("Could not delete in that location");
    } else if (p == FINGERPRINT_FLASHERR) {
      Serial.println("Error writing to flash");
    } else {
      Serial.print("Unknown error: 0x"); Serial.println(p, HEX);
    }

    return;
  }
}

uint8_t readnumber(void) {
  uint8_t num = 0;

  while (num == 0) {
    while (!Serial.available());
    num = Serial.parseInt();
  }
  return num;
}