int seq = 0;
int min = 100;
int max = 1020;

void setup() {
    Serial.begin(9600);
}

void loop() {
    int reading = analogRead(A0);

    if (reading > min && reading < max) {
        seq++;
        Serial.print(seq);
        Serial.print(",");
        Serial.println(reading);
    }
}
