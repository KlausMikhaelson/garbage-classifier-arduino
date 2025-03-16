#include "ServoManager.h"
#include "Config.h"
#include <Arduino.h>
#include "SoundManager.h"

namespace ServoManager {

Servo classificationServo;
Servo lidServo;

void attachServos() {
  classificationServo.attach(CLASSIFICATION_SERVO_PIN);
  lidServo.attach(LID_SERVO_PIN);
  lidServo.write(LID_CLOSED);
  classificationServo.write(POSITION_WASTE); // Default position
}

void setClassificationAngle(int angle) {
  classificationServo.write(angle);
  Serial.print("Rotating classification servo to ");
  Serial.print(angle);
  Serial.println(" degrees");
}

void operateLid() {
  Serial.println("Opening lid...");
  lidServo.write(LID_OPEN);
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(200);
  noTone(SPEAKER_PIN);
  delay(LID_OPEN_TIME - 200);
  Serial.println("Closing lid...");
  lidServo.write(LID_CLOSED);
  tone(SPEAKER_PIN, NOTE_C5, 200);
  delay(200);
  noTone(SPEAKER_PIN);
}

} // namespace ServoManager
