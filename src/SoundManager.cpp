#include "SoundManager.h"
#include "Config.h"
#include <Arduino.h>

namespace SoundManager {

void playStartupSound() {
  tone(SPEAKER_PIN, NOTE_C5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_E5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(120);
  tone(SPEAKER_PIN, NOTE_C6, 300);
  delay(300);
  noTone(SPEAKER_PIN);
}

void playSuccessSound() {
  tone(SPEAKER_PIN, NOTE_C5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_E5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_G5, 150);
  delay(150);
  tone(SPEAKER_PIN, NOTE_C6, 300);
  delay(300);
  noTone(SPEAKER_PIN);
}

void playErrorSound() {
  tone(SPEAKER_PIN, NOTE_C6, 200);
  delay(200);
  tone(SPEAKER_PIN, NOTE_E5, 200);
  delay(200);
  tone(SPEAKER_PIN, NOTE_C5, 400);
  delay(400);
  noTone(SPEAKER_PIN);
}

void playAlertSound() {
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(100);
  noTone(SPEAKER_PIN);
  delay(50);
  tone(SPEAKER_PIN, NOTE_G5, 100);
  delay(100);
  noTone(SPEAKER_PIN);
}

} // namespace SoundManager
