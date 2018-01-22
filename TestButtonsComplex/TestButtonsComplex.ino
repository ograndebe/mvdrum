
/*led, buttons and switches configuration*/
const unsigned long TURBO_PRESS_SIZE = 2000;
const unsigned long TURBO_INTERVAL = 700;
const unsigned long LONG_PRESS_SIZE = 4000;
const unsigned long MIN_PRESS_SIZE = 100;

const int UP_BUTTON = 2;
const int DOWN_BUTTON = 3;
const int LED_PIN = LED_BUILTIN; //D13

unsigned long upTimer = 0;
unsigned long downTimer = 0;
unsigned long currentMillis = 0;

const boolean USE_PULLUP = true; //change here

void setup() {
    // Set MIDI baud rate:
    Serial.begin(9600);
    // setup hi_hat switch
    pinMode(LED_PIN, OUTPUT);
    if (USE_PULLUP) {
        pinMode(DOWN_BUTTON, INPUT_PULLUP);
        pinMode(UP_BUTTON, INPUT_PULLUP);
    } else {
        pinMode(DOWN_BUTTON, INPUT);
        pinMode(UP_BUTTON, INPUT);
    }
}

void loop() {
    handleModeButtons();
}

boolean isButtonPressed(int buttonId) {
    if (USE_PULLUP) {
        return digitalRead(buttonId) == LOW;
    } else {
        return digitalRead(buttonId) == HIGH;
    }
}

void handleModeButtons() {
    currentMillis = millis();
    if(isButtonPressed (UP_BUTTON)) {
        if (upTimer == 0) {
            upTimer = currentMillis;
        } else {
            if ((currentMillis-upTimer) > TURBO_PRESS_SIZE && downTimer == 0) {
                upShortPress(currentMillis-upTimer);
                delay(TURBO_INTERVAL);
            }
        }
    } else {
        if (upTimer > 0) {
            int upPressed = currentMillis - upTimer;
            int downPressed = currentMillis - downTimer;
            if (upPressed >= LONG_PRESS_SIZE && downTimer > 0 && downPressed >=LONG_PRESS_SIZE ) {
                doubleLongPress(upPressed);
            } else if (upPressed >= MIN_PRESS_SIZE && upPressed < LONG_PRESS_SIZE) {
                upShortPress(upPressed);
            }
            upTimer = 0;
            downTimer = 0;
        }
    }
    if(isButtonPressed (DOWN_BUTTON)) {
        if (downTimer == 0) {
            downTimer = currentMillis;
        } else {
            if ((currentMillis-downTimer) > TURBO_PRESS_SIZE && upTimer == 0) {
                downShortPress(currentMillis-downTimer);
                delay(TURBO_INTERVAL);
            }
        }
    } else {
        if (downTimer > 0) {
            int upPressed = currentMillis - upTimer;
            int downPressed = currentMillis - downTimer;
            if (downPressed >= LONG_PRESS_SIZE && upTimer > 0 && upPressed >= LONG_PRESS_SIZE) {
                doubleLongPress(downPressed);
            } else if (downPressed >= MIN_PRESS_SIZE && downPressed < LONG_PRESS_SIZE) {
                downShortPress(downPressed);
            }
            downTimer = 0;
            upTimer = 0;
        }
    }
}

void upLongPress(int m) {
   Serial.print("upLongPress ");
   Serial.println(m);
}

void upShortPress(int m) {
    Serial.print("upShortPress ");
    Serial.println(m);
}

void downShortPress(int m) {
    Serial.print("downShortPress ");
    Serial.println(m);
}

void doubleLongPress(int m) {
    Serial.print("doubleLongPress ");
    Serial.println(m);
}

void downLongPress(int m) {
    Serial.print("downLongPress ");
    Serial.println(m);
}
