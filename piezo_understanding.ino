int seq = 0;


void setup() {
    Serial.begin(9600);
}

void loop() {
    int reading = analogRead(A0);

    if (reading > 0) {
        seq++;
        Serial.print(seq);
        Serial.print(",");
        Serial.println(reading);
    }
}
