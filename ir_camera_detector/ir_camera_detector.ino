// pins
const int ledPin    = 3;   // drives 2N2222A base
const int sensorPin = A0;  // IR photodiode junction
const int detectLed = 13;  // onboard LED indicator

// tunables
int pulseDelay = 200;      // microseconds: ON/OFF IR settle time
int threshold  = 30;       // diff needed to count as "detected"


unsigned long startTime;

void setup() {
  pinMode(ledPin, OUTPUT);
  pinMode(detectLed, OUTPUT);
  Serial.begin(115200);

  startTime = millis();
  Serial.println("ms,diff"); // csv header
}

int readDiff() {
  // LED OFF (ambient only)
  digitalWrite(ledPin, LOW);
  delayMicroseconds(pulseDelay);
  int dark = analogRead(sensorPin);

  // LED ON (our IR + ambient)
  digitalWrite(ledPin, HIGH);
  delayMicroseconds(pulseDelay);
  int lit = analogRead(sensorPin);

  return (lit - dark);   // remove ambient
}

void loop() {
  int diff = readDiff();

  unsigned long t = millis() - startTime;

  Serial.print(t);
  Serial.print(",");
  Serial.println(diff);

  // simple threshold trigger
  if (diff > threshold) {
    digitalWrite(detectLed, HIGH);
  } else {
    digitalWrite(detectLed, LOW);
  }
}
