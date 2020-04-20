int seq = 0;


void setup() {
    Serial.begin(9600);
}

void loop() {
    seq++;
    Serial.print(seq);
    Serial.print(",");
    int reading = analogRead(A0);
    Serial.println(reading);
}
