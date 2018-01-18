#include <EEPROM.h>


/*############################# BUTTONS CONTROL ################################*/
/*led, buttons and switches configuration*/
const unsigned long TURBO_PRESS_SIZE = 3000;
const unsigned long TURBO_INTERVAL = 700;
const unsigned long LONG_PRESS_SIZE = 2000;
const unsigned long MIN_PRESS_SIZE = 100;

const int UP_BUTTON = 2;
const int DOWN_BUTTON = 3;
unsigned long upTimer = 0;
unsigned long downTimer = 0;

const boolean USE_PULLUP = true; //change here


boolean isButtonPressed(int buttonId) {
    if (USE_PULLUP) {
        return digitalRead(buttonId) == LOW;
    } else {
        return digitalRead(buttonId) == HIGH;
    }
}

void handleModeButtons() {
    if(isButtonPressed (UP_BUTTON)) {
        if (upTimer == 0) {
            upTimer = millis();
        } else {
            if ((millis()-upTimer) > TURBO_PRESS_SIZE && downTimer == 0) {
                upShortPress(millis()-upTimer);
                delay(TURBO_INTERVAL);
            }
        }
    } else {
        if (upTimer > 0) {
            int upPressed = millis() - upTimer;
            if (upPressed >= LONG_PRESS_SIZE && downTimer >=LONG_PRESS_SIZE ) {
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
            downTimer = millis();
        } else {
            if ((millis()-downTimer) > TURBO_PRESS_SIZE && upTimer == 0) {
                downShortPress(millis()-downTimer);
                delay(TURBO_INTERVAL);
            }
        }
    } else {
        if (downTimer > 0) {
            int downPressed = millis() - downTimer;
            if (downPressed >= LONG_PRESS_SIZE && upTimer >= LONG_PRESS_SIZE) {
                doubleLongPress(downPressed);
            } else if (downPressed >= MIN_PRESS_SIZE && downPressed < LONG_PRESS_SIZE) {
                downShortPress(downPressed);
            }
            downTimer = 0;
            upTimer = 0;
        }
    }
}


void setupButtons() {
    if (USE_PULLUP) {
        pinMode(DOWN_BUTTON, INPUT_PULLUP);
        pinMode(UP_BUTTON, INPUT_PULLUP);
    } else {
        pinMode(DOWN_BUTTON, INPUT);
        pinMode(UP_BUTTON, INPUT);
    }
}


/*############################# BUTTONS CONTROL ################################*/



/*############################ PARAMETERS #####################################*/
/*Below sensive parameters, be careful*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int MIDI_CMD_CC = 0xB0; 
const int KNOCK_THRESHOLD = 100;  //sensor range 0 to 1023
const int MAX_MIDI_VELOCITY = 127;
/*Any value between this below will be half hi-hat*/
const int ANALOG_HI_HAT_LOW_LIMIT = 10;
const int ANALOG_HI_HAT_HIGH_LIMIT = 117;
const unsigned long ANALOG_HI_HAT_PEDAL_TIME = 500; // milliseconds
/*############################ PARAMETERS #####################################*/

/*########################### CONSTANTS  ####################################*/
const int MODE_HIHAT_SWITCH = 1;
const int MODE_HIHAT_3P     = 2;
const int MODE_HIHAT_CC     = 3;

const int IDX_ANALOG_INPUT      = 0;
const int IDX_LAST_KNOCK_BUFFER = 1;
const int IDX_NOTE              = 2;
const int IDX_TYPE              = 3;
const int IDX_SWITCH            = 4;
const int TYPE_NORMAL           = 10;
const int TYPE_HIHAT            = 20;
const int TYPE_HIHAT_PEDAL      = 30;
/*########################### CONSTANTS  ####################################*/

/*#################### CONFIGURATION IS HERE #################*/
const int CONF_MATRIX_SIZE = 16; 
int CONF_MATRIX [16][5] {
//  {IDX_ANALOG_INPUT, IDX_LAST_KNOCK_BUFFER, IDX_NOTE, IDX_TYPE,    IDX_SWITCH}
    {A0              , 0                    , 46      , TYPE_HIHAT , 7},
    {A1              , 0                    , 36      , TYPE_NORMAL, 0},
    {A2              , 0                    , 38      , TYPE_NORMAL, 0},
    {A3              , 0                    , 48      , TYPE_NORMAL, 0},
    {A4              , 0                    , 45      , TYPE_NORMAL, 0},
    {A5              , 0                    , 41      , TYPE_NORMAL, 0},
    {A6              , 0                    , 42      , TYPE_NORMAL, 0},
    {A7              , 0                    , 43      , TYPE_NORMAL, 0},
    {A8              , 0                    , 44      , TYPE_NORMAL, 0},
    {A9              , 0                    , 47      , TYPE_NORMAL, 0},
    {A10             , 0                    , 67      , TYPE_NORMAL, 0},
    {A11             , 0                    , 65      , TYPE_NORMAL, 0},
    {A12             , 0                    , 62      , TYPE_NORMAL, 0},
    {A13             , 0                    , 51      , TYPE_NORMAL, 4},
    {A14             , 0                    , 49      , TYPE_NORMAL, 5},
    {A15             , 0                    , 57      , TYPE_NORMAL, 6},
};
int hihatMode = MODE_HIHAT_SWITCH;
int THREE_PHASE_HIHAT_NOTES [4] {50,51,52,53};
/*#################### CONFIGURATION IS HERE #################*/

/*led configuration*/
const int LED_PIN = LED_BUILTIN; //D13

/*Variables*/
int sensorReading = 0;   
int calcVelocity = 0;

/*HiHat*/
int lastHiHatPosition = 0;
unsigned long analogHiHatOpenedTime = 0;

/*Learn Mode*/
char currentMode = 'P'; // [P]lay [L]earn
int lastPlayedIndex = 0;
int currentNote = 0;

void setup() {
    // Set MIDI baud rate:
    Serial.begin(31250);
    // setup hi_hat switch
    pinMode(LED_PIN, OUTPUT);
    setupButtons();
    
    //read last saved NOTES
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        currentNote = EEPROM.read(idx);
        if (currentNote != 0 && currentNote >= 1 && currentNote <= 128) { //only reads if != 0 for default values
            CONF_MATRIX[idx][IDX_NOTE] = currentNote-1;
        }
        if (CONF_MATRIX[idx][IDX_SWITCH] != 0) {
            pinMode(CONF_MATRIX[idx][IDX_SWITCH], INPUT_PULLUP);
        }
    }
    for (int idx = 0; idx < 4; idx++) {
         currentNote = EEPROM.read(idx+20);
         if (currentNote != 0 && currentNote >= 1 && currentNote <= 128) { 
             THREE_PHASE_HIHAT_NOTES[idx] = currentNote-1;
         }         
    }
}

void loop() {
    handleModeButtons();

    for (int analogInput = 0; analogInput < CONF_MATRIX_SIZE; analogInput++) {
        int type = CONF_MATRIX[analogInput][IDX_TYPE];
        if ( type == TYPE_NORMAL) {
            calcVelocity = detectKnock(analogInput);
            if (calcVelocity > 0) {
                handleNormalPad(analogInput, calcVelocity);
            } 
            handleChoke(analogInput);
        } else if (type == TYPE_HIHAT_PEDAL){
            calcVelocity = readAnalog(analogInput);
            handleHihatAnalogPedal(analogInput, calcVelocity);
        } else if (type == TYPE_HIHAT) {
            calcVelocity = detectKnock(analogInput);
            handleHiHatBeat(analogInput, calcVelocity);
        }
    }

}

void handleNormalPad(int idx, int velocity) {
    lastPlayedIndex = idx;
    noteOn(CONF_MATRIX[idx][IDX_NOTE], velocity);
}

void handleChoke (int idx) {
    if (digitalRead(CONF_MATRIX[idx][IDX_SWITCH]) == LOW) {
        noteOff(CONF_MATRIX[idx][IDX_NOTE]);
    }
}

void handleHihatAnalogPedal(int idx, int value) {
    CONF_MATRIX[idx][IDX_LAST_KNOCK_BUFFER] = value;
    if (hihatMode == MODE_HIHAT_CC) {
        sendControlChange(CONF_MATRIX[idx][IDX_NOTE],value);
    } else if (hihatMode == MODE_HIHAT_3P) {
        if (lastHiHatPosition < ANALOG_HI_HAT_HIGH_LIMIT && value >= ANALOG_HI_HAT_HIGH_LIMIT ) {
            //opened
            analogHiHatOpenedTime = millis();
        } else if (lastHiHatPosition > ANALOG_HI_HAT_LOW_LIMIT && value <= ANALOG_HI_HAT_LOW_LIMIT) {
            //closed
            analogHiHatOpenedTime = millis() - analogHiHatOpenedTime;
            if (analogHiHatOpenedTime <= ANALOG_HI_HAT_PEDAL_TIME) {
                noteOn(THREE_PHASE_HIHAT_NOTES[3], 127-int((float(analogHiHatOpenedTime)/float(ANALOG_HI_HAT_PEDAL_TIME))*127.0));
            }
        }
    }
    lastHiHatPosition = value;
}

void handleHiHatBeat(int idx, int velocity) {
    //handle switch stepped
    if (hihatMode == MODE_HIHAT_SWITCH) {
        int position = 0;
        if (digitalRead(CONF_MATRIX[idx][IDX_SWITCH]) == HIGH) position = 127;

        if (position == 0 && lastHiHatPosition!= position ) {
            //stepped
            noteOn(THREE_PHASE_HIHAT_NOTES[3], 127);
        }
    }
    //handle hihat beat indeed
    if (velocity > 0) {
        if (hihatMode == MODE_HIHAT_SWITCH || hihatMode == MODE_HIHAT_3P) {
            if (lastHiHatPosition <= ANALOG_HI_HAT_LOW_LIMIT) {
                //closed
                noteOn(THREE_PHASE_HIHAT_NOTES[0], velocity);
            } else if (lastHiHatPosition >= ANALOG_HI_HAT_HIGH_LIMIT ) {
                //opened
                noteOn(THREE_PHASE_HIHAT_NOTES[2], velocity);
            } else {
                //half
                noteOn(THREE_PHASE_HIHAT_NOTES[1], velocity);
            }
        }  else {
            noteOn(CONF_MATRIX[idx][IDX_NOTE], velocity);
        }
    }
    lastPlayedIndex = idx;
}


void upShortPress() {
    if (currentMode == 'L') {
        currentNote = CONF_MATRIX[lastPlayedIndex][IDX_NOTE];
        currentNote++;
        if (currentNote > 127) currentNote = 0;
        CONF_MATRIX[lastPlayedIndex][IDX_NOTE] = currentNote;
        noteOn(currentNote, MAX_MIDI_VELOCITY);
    }
}

void downShortPress() {
    if (currentMode == 'L') {
        currentNote = CONF_MATRIX[lastPlayedIndex][IDX_NOTE];
        currentNote--;
        if (currentNote < 0) currentNote = 127;
        CONF_MATRIX[lastPlayedIndex][IDX_NOTE] = currentNote;
        noteOn(currentNote, MAX_MIDI_VELOCITY);
    }
}

void doubleLongPress() {
    if (currentMode == 'P') {
        currentMode = 'L';
        digitalWrite(LED_PIN, HIGH);
    } else {
        currentMode = 'P';
        digitalWrite(LED_PIN, LOW);

        //write current selected notes
        for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
            EEPROM.update(idx, CONF_MATRIX[idx][IDX_NOTE]+1);
        }

        for (int idx = 0; idx < 4; idx++) {
            EEPROM.update(idx+20, THREE_PHASE_HIHAT_NOTES[idx]+1);
        }
    }
}


void noteOn(int pitch, int velocity) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(velocity);
}
void noteOff(int pitch) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(0);
}
void sendControlChange(int cc, int value) {
    Serial.write(MIDI_CMD_CC);
    Serial.write(cc);
    Serial.write(value);
}

int detectKnock(int analogInputIdx) {
    sensorReading = analogRead(CONF_MATRIX[analogInputIdx][IDX_ANALOG_INPUT]);
    if (sensorReading >= KNOCK_THRESHOLD && CONF_MATRIX[analogInputIdx][IDX_LAST_KNOCK_BUFFER] < sensorReading) {
        CONF_MATRIX[analogInputIdx][IDX_LAST_KNOCK_BUFFER] = sensorReading; // store last reading for knock only if value is greater
        return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
    } else {
        CONF_MATRIX[analogInputIdx][IDX_LAST_KNOCK_BUFFER] = sensorReading; // store last reading for knock only if value is greater
        return 0;
    }
}

int readAnalog(int analogInputIdx) {
    sensorReading = analogRead(CONF_MATRIX[analogInputIdx][IDX_ANALOG_INPUT]);
    if (sensorReading >= KNOCK_THRESHOLD) {
        CONF_MATRIX[analogInputIdx][IDX_LAST_KNOCK_BUFFER] = sensorReading; // store last reading for knock only if value is greater
        return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
    } else {
        CONF_MATRIX[analogInputIdx][IDX_LAST_KNOCK_BUFFER] = sensorReading; // store last reading for knock only if value is greater
        return 0;
    }
}
