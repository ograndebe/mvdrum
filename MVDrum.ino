#include <EEPROM.h>

/*######## IMPORTANT ############*/
/*Change what setup you want HERE*/
/*######## IMPORTANT ############*/
// #define SETUP_A
// #define SETUP_B
// #define SETUP_C
#define SETUP_D

/*
Parameters
*/
/*Below sensive parameters, be careful*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int KNOCK_THRESHOLD = 100;  //sensor range 0 to 1023
const int MAX_MIDI_VELOCITY = 127;
const unsigned long LONG_PRESS_SIZE = 2000; // milliseconds
/*Any value between this below will be half hi-hat*/
const int ANALOG_HI_HAT_LOW_LIMIT = 100;
const int ANALOG_HI_HAT_HIGH_LIMIT = 900;
const unsigned long ANALOG_HI_HAT_PEDAL_TIME = 500; // milliseconds

const int IDX_ANALOG_INPUT = 0;
const int IDX_LAST_KNOCK_BUFFER = 1;
const int IDX_NOTE = 2;
/*Array to define witch note associated with each drum note*/

/*########### DEV #################*/
const int IDX_TYPE = 3;
const int IDX_SWITCH = 4;
const int TYPE_NORMAL           = 10;
const int TYPE_HIHAT            = 20;
const int TYPE_HIHAT_CC_PEDAL   = 30;
const int TYPE_HIHAT_3P_PEDAL   = 40;

const int CONF_MATRIX_SIZE = 8; 
int CONF_MATRIX [8][5] {
//  {IDX_ANALOG_INPUT, IDX_LAST_KNOCK_BUFFER, IDX_NOTE, IDX_TYPE,    IDX_SWITCH}
    {A0              , 0                    , 0       , TYPE_HIHAT , 7},
    {A1              , 0                    , 0       , TYPE_NORMAL, 0},
    {A2              , 0                    , 0       , TYPE_NORMAL, 0},
    {A3              , 0                    , 0       , TYPE_NORMAL, 0},
    {A4              , 0                    , 0       , TYPE_NORMAL, 0},
    {A5              , 0                    , 0       , TYPE_NORMAL, 4},
    {A6              , 0                    , 0       , TYPE_NORMAL, 5},
    {A7              , 0                    , 0       , TYPE_NORMAL, 6},
};

const int THREE_PHASE_HIHAT [3] {50,51,52};
/*########### DEV #################*/

/*led, buttons and switches configuration*/
const int UP_BUTTON = 2;
const int DOWN_BUTTON = 3;
const int LED_PIN = LED_BUILTIN; //D13

/*Variables*/
int sensorReading = 0;   
int calcVelocity = 0;

/*HiHat*/
int hiHatSwitchState = 0;
int lastHiHatPosition = LOW;
int analogHiHatPosition = 100;
unsigned long analogHiHatOpenedTime = 0;

/*Learn Mode*/
char currentMode = 'P'; // [P]lay [L]earn
int lastPlayedIndex = 0;
unsigned long upTimer = 0;
unsigned long downTimer = 0;
boolean upActive = false;
boolean downActive = false;
int currentNote = 0;

void setup() {
    // Set MIDI baud rate:
    Serial.begin(31250);
    // setup hi_hat switch
    pinMode(LED_PIN, OUTPUT);
    pinMode(DOWN_BUTTON, INPUT_PULLUP);
    pinMode(UP_BUTTON, INPUT_PULLUP);
    
    //read last saved NOTES
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        currentNote = EEPROM.read(idx);
        if (currentNote != 0) { //only reads if != 0 for default values
            CONF_MATRIX[idx][IDX_NOTE] = currentNote-1;
        }
    }
}

void loop() {
    handleModeButtons();

    for (int analogInput = 0; analogInput < CONF_MATRIX_SIZE; analogInput++) {
        int type = CONF_MATRIX[analogInput][IDX_TYPE];
        if ( type == TYPE_NORMAL || type == TYPE_HIHAT) {
            calcVelocity = detectKnock(analogInput);
            if (calcVelocity > 0) {
                if (type == TYPE_NORMAL) {
                    handleNormalPad(analogInput, calcVelocity);
                } else {
                    handleSimpleHiHat(analogInput, calcVelocity);
                }
            } else {
                //TODO handleChoke();
            }
        } else {
            calcVelocity = readAnalog(analogInput);
            if (type == TYPE_HIHAT_CC_PEDAL) {
                handleHihatCCPedal()
            } else {

            }
        }
    }

    handleSensors();
    handleHiHat();
    handleCHOKE();
}

void handleNormalPad(int idx, int velocity) {
    lastPlayedIndex = idx;
    noteOn(CONF_MATRIX[idx][IDX_NOTE], velocity);
}

void handleSimpleHiHat(int idx, int velocity) {
    lastPlayedIndex = idx;

    hiHatSwitchState = digitalRead(CONF_MATRIX[idx][IDX_SWITCH]);
    if(hiHatSwitchState == LOW) {
        //on
        noteOn(THREE_PHASE_HIHAT[2], velocity);
    } else {
        //off
        noteOn(THREE_PHASE_HIHAT[0], velocity);
    }

    if (lastHiHatPosition != hiHatSwitchState) {
        noteOn(THREE_PHASE_HIHAT[1], MAX_MIDI_VELOCITY); //no velocity
    }
    lastHiHatPosition = hiHatSwitchState;
}

void handleHihatCCPedal(int idx, int paramValue) {
    //TODO send control change
}

void handleModeButtons() {
    if(digitalRead(UP_BUTTON) == LOW) {
        if (upActive == false) {
            upActive = true;
            upTimer = millis();
        }    
    } else {
        if (upActive == true) {
            if (upTimer >= LONG_PRESS_SIZE) {
                upLongPress();
            } else {
                upShortPress();
            }
            upActive = false;
            upTimer = 0;
        }
    }
    if(digitalRead(DOWN_BUTTON) == LOW) {
        if (downActive == false) {
            downActive = true;
            downTimer = millis();
        }    
    } else {
        if (downActive == true) {
            if (downTimer >= LONG_PRESS_SIZE && upTimer >= LONG_PRESS_SIZE) {
                doubleLongPress();
            } else if (downTimer >= LONG_PRESS_SIZE) {
                downLongPress();
            } else {
                downShortPress();
            }
            downActive = false;
            downTimer = 0;
            upActive = false;
            upTimer = 0;
        }
    }
}

void handleCHOKE() {
    for (int idx = 0; idx < CHOKE_MATRIX_SIZE; idx++) {
        sensorReading = digitalRead(CHOKE_MATRIX[idx][0]);
        if (sensorReading != CHOKE_MATRIX[idx][1]) {
            if (sensorReading == LOW) {
                //NoteOff
                noteOn(CONF_MATRIX[idx][IDX_NOTE], 0); //first 3 analog inputs
            }
            CHOKE_MATRIX[idx][1] = sensorReading;
        }
    }
}


void upLongPress() {
    if (currentMode == 'P') {
        currentMode = 'L';
        digitalWrite(LED_PIN, HIGH);
    } else {
        currentMode = 'P';
        digitalWrite(LED_PIN, LOW);
    }
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
    //write current selected notes
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        EEPROM.update(idx, CONF_MATRIX[idx][IDX_NOTE]);
    }
}

void downLongPress() {
    //None for now
    downShortPress();
}


void handleSensors() {
    for (int analogInput = 0; analogInput < ANALOG_INPUTS_SIZE_TO_SCAN; analogInput++) {
        calcVelocity = detectKnock(analogInput);
        if (calcVelocity > 0) {
            lastPlayedIndex = analogInput;
            noteOn(CONF_MATRIX[analogInput][IDX_NOTE], calcVelocity);
        }
    }
}

void handleHiHat() {
    if (HAS_ANALOG_HIHAT == true) {
        sensorReading = analogRead(CONF_MATRIX[IDX_HI_HAT_PEDAL][IDX_ANALOG_INPUT]);
        if (sensorReading <= ANALOG_HI_HAT_LOW_LIMIT) {
            //closed
            if (analogHiHatPosition != 0 && analogHiHatOpenedTime <= ANALOG_HI_HAT_PEDAL_TIME) {
                //Note on
                lastPlayedIndex = IDX_HI_HAT_PEDAL;
                //calculate velocity based how fastest the hi-hat is closed
                calcVelocity = int((float(analogHiHatOpenedTime)/float(ANALOG_HI_HAT_PEDAL_TIME))*127.0);
                noteOn(CONF_MATRIX[lastPlayedIndex][IDX_NOTE], calcVelocity);
            }
            analogHiHatPosition = 0;
        } else if (sensorReading <= ANALOG_HI_HAT_HIGH_LIMIT) {
            //half
            analogHiHatPosition = 1;
        } else {
            //opened
            analogHiHatPosition = 2;
            analogHiHatOpenedTime = millis();
        }
    }

    //handle hi-hat
    calcVelocity = detectKnock(IDX_HI_HAT_OPENED);
    if (calcVelocity > 0) {
        hiHatSwitchState = digitalRead(HI_HAT_SWITCH);
        if (hiHatSwitchState == LOW || analogHiHatPosition == 0) {
            lastPlayedIndex = IDX_HI_HAT_CLOSED;
        } else if (hiHatSwitchState == HIGH || analogHiHatPosition == 2){
            lastPlayedIndex = IDX_HI_HAT_OPENED;
        } else if (analogHiHatPosition == 1) {
            lastPlayedIndex = IDX_HI_HAT_HALF;
        }
        noteOn(CONF_MATRIX[lastPlayedIndex][IDX_NOTE], calcVelocity);
    }
    
    if (lastHiHatPosition != hiHatSwitchState) {
        lastPlayedIndex = IDX_HI_HAT_PEDAL;
        noteOn(CONF_MATRIX[IDX_HI_HAT_PEDAL][IDX_NOTE], MAX_MIDI_VELOCITY); //no velocity
    }

    lastHiHatPosition = hiHatSwitchState;
}

void noteOn(int pitch, int velocity) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(velocity);
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