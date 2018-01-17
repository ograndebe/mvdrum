
/*led, buttons and switches configuration*/
const unsigned long LONG_PRESS_SIZE = 3000;

const int UP_BUTTON = 2;
const int DOWN_BUTTON = 3;
const int LED_PIN = LED_BUILTIN; //D13

unsigned long upTimer = 0;
unsigned long downTimer = 0;
boolean upActive = false;
boolean downActive = false;

const boolean USE_PULLUP = true; //trocar aqui

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
    boolean isUpShort = false;
    boolean isUpLong = false;
    boolean isDownShort = false;
    boolean isDownLong = false;
    if(isButtonPressed (UP_BUTTON)) {
        if (upActive == false) {
            upActive = true;
            upTimer = millis();
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
    if(isButtonPressed (DOWN_BUTTON)) {
        if (downActive == false) {
            downActive = true;
            downTimer = millis();
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

