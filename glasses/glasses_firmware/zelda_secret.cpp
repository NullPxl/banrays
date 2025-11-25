#include <Arduino.h>
#include "zelda_secret.h"

const int BUZZER_PIN = 12;
const int CHANNEL = 0;
const int RESOLUTION = 8;
const int VOLUME = 25;
// note:this is blocking for about a second bc of delay() calls. but i think that's okay for now
// if i really end up caring about it just check time elapsed or smthng with millis()

// TODO: don't play sound again for same MAC address (it rotates every little bit anyway)

void zeldaSecret() {
  ledcWriteNote(CHANNEL, NOTE_G, 5);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_Fs, 5);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_Eb, 5);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_A, 4);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_Gs, 4);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_E, 5);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_Gs, 5);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteNote(CHANNEL, NOTE_C, 6);
  ledcWrite(CHANNEL, VOLUME);
  delay(125);

  ledcWriteTone(CHANNEL, 0);
}

void setupZeldaSound() {
  ledcAttachPin(BUZZER_PIN, CHANNEL);
  ledcSetup(CHANNEL, 2000, RESOLUTION);
}
