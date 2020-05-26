int seq = 0;
int min = 1;
int max = 1023;

void setup() {
    Serial.begin(9600);
}

void loop() {
    int reading = analogRead(A15);

    if (reading > min && reading < max) {
        Serial.print(millis());
        Serial.print(",");
        Serial.println(reading);
    }
}
