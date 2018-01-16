
/*led, buttons and switches configuration*/
const unsigned long LONG_PRESS_SIZE = 3000;
const unsigned long LONG_HIT_SIZE = 5000;
const unsigned long HIT_DELAY = 700;

const int UP_BUTTON = 2;
const int DOWN_BUTTON = 3;
const int LED_PIN = LED_BUILTIN; //D13

unsigned long upTimer = 0;
unsigned long downTimer = 0;
boolean upActive = false;
boolean downActive = false;

void setup() {
    // Set MIDI baud rate:
    Serial.begin(9600);
    // setup hi_hat switch
    pinMode(LED_PIN, OUTPUT);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
}

void loop() {
    handleModeButtons();
}

void handleModeButtons() {
    boolean isUpShort = false;
    boolean isUpLong = false;
    boolean isDownShort = false;
    boolean isDownLong = false;
    boolean isHitUp = false;
    boolean isHitDown = false;
    if(digitalRead(UP_BUTTON) == LOW) {
        if (upActive == false) {
            upActive = true;
            upTimer = millis();
        } else  {
            if ((millis()-upTimer)> LONG_HIT_SIZE) {
                isHitUp = true;
            }
        }
    } else {
        if (upActive == true) {
            if ((millis()-upTimer) >= LONG_PRESS_SIZE) {
                isUpLong = true;
            } else {
                isUpShort = true;
            }
            upActive = false;
            upTimer = 0;
        }
    }
    if(digitalRead(DOWN_BUTTON) == LOW) {
        if (downActive == false) {
            downActive = true;
            downTimer = millis();
        } else  {
            if ((millis()-downTimer)> LONG_HIT_SIZE) {
                isHitDown = true;
            }
        }
    } else {
        if (downActive == true) {
            if ((millis()-downTimer) >= LONG_PRESS_SIZE) {
                isDownLong = true;
            } else {
                isDownShort = true;
            }
            downActive = false;
            downTimer = 0;
            upActive = false;
            upTimer = 0;
        }
    }

    if (isDownLong == true && isUpLong == true) doubleLongPress();
    else if (isDownLong == true) downLongPress();
    else if (isUpLong == true) upLongPress();
    else if (isDownShort == true) downShortPress();
    else if (isUpShort == true) upShortPress();
    else if (isHitUp == true) {
        delay(HIT_DELAY); 
        upShortPress();
    } else if (isHitDown == true) {
        delay(HIT_DELAY); 
        downShortPress();
    }
}

void upLongPress() {
   Serial.println("upLongPress ");
}

void upShortPress() {
    Serial.println("upShortPress ");
}

void downShortPress() {
    Serial.println("downShortPress ");
}

void doubleLongPress() {
    Serial.println("doubleLongPress ");
}

void downLongPress() {
    Serial.println("downLongPress ");
}

