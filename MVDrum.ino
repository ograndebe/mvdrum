#include <EEPROM.h>

/*######## IMPORTANT ############*/
/*Change what setup you want HERE*/
/*######## IMPORTANT ############*/
#define SETUP_A
//#define SETUP_C



/*
Parameters
*/
/*Below sensive parameters, be careful*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int KNOCK_THRESHOLD = 100;  
const int MAX_MIDI_VELOCITY = 127;

const int IDX_ANALOG_INPUT = 0;
const int IDX_LAST_KNOCK_BUFFER = 1;
const int IDX_NOTE = 2;
/*Array to define witch note associated with each drum note*/

#if defined(SETUP_A)
const int CONF_MATRIX_SIZE = 10; 
int CONF_MATRIX [10][3] {
    //{IDX_ANALOG_INPUT, IDX_LAST_KNOCK_BUFFER, IDX_NOTE}
    {A0,0,0}, 
    {A1,0,0}, 
    {A2,0,0}, 
    {A3,0,0}, 
    {A4,0,0}, 
    {A5,0,0}, 
    {A6,0,0}, 
    {A7,0,0},
    {A7,0,0},
    {A7,0,0}
};
const int ANALOG_INPUTS_SIZE = 7; //not run last analog input
#endif
#if defined( SETUP_C)
const int CONF_MATRIX_SIZE = 18; 
int CONF_MATRIX [18][3] {
    //{IDX_ANALOG_INPUT, IDX_LAST_KNOCK_BUFFER, IDX_NOTE}
    {A0,0,0}, 
    {A1,0,0}, 
    {A2,0,0}, 
    {A3,0,0}, 
    {A4,0,0}, 
    {A5,0,0}, 
    {A6,0,0}, 
    {A7,0,0}, 
    {A8,0,0}, 
    {A9,0,0}, 
    {A10,0,0}, 
    {A11,0,0}, 
    {A12,0,0}, 
    {A13,0,0}, 
    {A14,0,0}, 
    {A15,0,0}, 
    {A16,0,0},
    {A16,0,0},
    {A16,0,0}
};
const int ANALOG_INPUTS_SIZE = 15; //not run last analog input
#endif


const int IDX_HI_HAT_OPENED = ANALOG_INPUTS_SIZE; 
const int IDX_HI_HAT_CLOSED = IDX_HI_HAT_OPENED+1; 
const int IDX_HI_HAT_PEDAL = IDX_HI_HAT_CLOSED+1;

const int HI_HAT_SWITCH = 2;
const int UP_SWITCH = 3;
const int DOWN_SWITCH = 4;

const unsigned long LONG_PRESS_SIZE = 2000;
const int LED_PIN =  LED_BUILTIN;

/*Variables*/
int sensorReading = 0;   
int calcVelocity = 0;

/*HiHat*/
int hiHatSwitchState = 0;
int lastHiHatPosition = LOW;

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
    pinMode(HI_HAT_SWITCH, INPUT);
    pinMode(DOWN_SWITCH, INPUT);
    pinMode(UP_SWITCH, INPUT);

    //read last saved NOTES
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        CONF_MATRIX[idx][IDX_NOTE] = EEPROM.read(idx);
    }
}

void loop() {
    handleModeButtons();
    handleSensors();
    handleHiHat();
}

void handleModeButtons() {
    if(digitalRead(UP_SWITCH) == HIGH) {
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
    if(digitalRead(DOWN_SWITCH) == HIGH) {
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
}


void handleSensors() {
    for (int analogInput = 0; analogInput < ANALOG_INPUTS_SIZE; analogInput++) {
        calcVelocity = detectKnock(analogInput);
        if (calcVelocity > 0) {
            lastPlayedIndex = analogInput;
            noteOn(CONF_MATRIX[analogInput][IDX_NOTE], calcVelocity);
        }
    }
}

void handleHiHat() {
    //handle hi-hat
    calcVelocity = detectKnock(IDX_HI_HAT_OPENED);
    if (calcVelocity > 0) {
        hiHatSwitchState = digitalRead(HI_HAT_SWITCH);
        if (hiHatSwitchState == HIGH) {
            lastPlayedIndex = IDX_HI_HAT_CLOSED;
            noteOn(CONF_MATRIX[IDX_HI_HAT_CLOSED][IDX_NOTE], calcVelocity);
        } else {
            lastPlayedIndex = IDX_HI_HAT_OPENED;
            noteOn(CONF_MATRIX[IDX_HI_HAT_OPENED][IDX_NOTE], calcVelocity);
        }
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
