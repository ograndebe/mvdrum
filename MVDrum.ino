#include <EEPROM.h>

/*Drum note Map*/
const int ACOUSTIC_BASS_DRUM = 35;  
//const int BASS_DRUM_1        = 36;  
//const int SIDE_STICK         = 37;  
const int ACOUSTIC_SNARE     = 38;  
//const int HAND_CLAP          = 39;  
//const int ELECTRIC_SNARE     = 40;  
//const int LOW_FLOOR_TOM      = 41;  
const int CLOSED_HI_HAT      = 42;  
//const int HIGH_FLOOR_TOM     = 43;  
const int PEDAL_HI_HAT       = 44;  
//const int LOW_TOM            = 45;  
const int OPEN_HI_HAT        = 46;  
//const int LOW_MID_TOM        = 47;  
//const int HI_MID_TOM         = 48;  
const int CRASH_CYMBAL_1     = 49;  
const int HIGH_TOM           = 50;  
//const int RIDE_CYMBAL_1      = 51;  
//const int CHINESE_CYMBAL     = 52;  
//const int RIDE_BELL          = 53;  
//const int TAMBOURINE         = 54;  
//const int SPLASH_CYMBAL      = 55;  
//const int COWBELL            = 56;  
const int CRASH_CYMBAL_2     = 57;  
//const int VIBRASLAP          = 58;  
const int RIDE_CYMBAL_2      = 59;  
//const int HI_BONGO           = 60;  
//const int LOW_BONGO          = 61;  
//const int MUTE_HI_CONGA      = 62;  
//const int OPEN_HI_CONGA      = 63;  
//const int LOW_CONGA          = 64;  
//const int HIGH_TIMBALE       = 65;  
//const int LOW_TIMBALE        = 66;  
//const int HIGH_AGOGO         = 67;  
//const int LOW_AGOGO          = 68;  
//const int CABASA             = 69;  
//const int MARACAS            = 70;  
//const int SHORT_WHISTLE      = 71;  
//const int LONG_WHISTLE       = 72;  
//const int SHORT_GUIRO        = 73;  
//const int LONG_GUIRO         = 74;  
//const int CLAVES             = 75;  
//const int HI_WOOD_BLOCK      = 76;  
//const int LOW_WOOD_BLOCK     = 77;  
//const int MUTE_CUICA         = 78;  
//const int OPEN_CUICA         = 79;  
//const int MUTE_TRIANGLE      = 80;  
//const int OPEN_TRIANGLE      = 81;


/*
Parameters
*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int KNOCK_THRESHOLD = 100;  
const int MAX_MIDI_VELOCITY = 127;

const int IDX_ANALOG_INPUT = 0;
const int IDX_LAST_KNOCK_BUFFER = 1;
const int IDX_NOTE = 2;
/*Array to define witch note associated with each drum note*/
int CONF_MATRIX [10][3] {
    //{IDX_ANALOG_INPUT, IDX_NOTE}
    {A0,0,ACOUSTIC_BASS_DRUM}, 
    {A1,0,ACOUSTIC_SNARE    }, 
    {A2,0,ACOUSTIC_SNARE    }, 
    {A3,0,HIGH_TOM          }, 
    {A4,0,CRASH_CYMBAL_1    }, 
    {A5,0,RIDE_CYMBAL_2     }, 
    {A6,0,CRASH_CYMBAL_2    }, 
    {A7,0,OPEN_HI_HAT       },
    {A7,0,CLOSED_HI_HAT     },
    {A7,0,PEDAL_HI_HAT      }
};
const int CONF_MATRIX_SIZE = 10;
const int ANALOG_INPUTS_SIZE = 7; //not run last analog input
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
int switchState = 0;
int lastHiHatPosition = LOW;
byte temporaryValue = 0;
char currentMode = 'P';
unsigned long upTimer = 0;
unsigned long downTimer = 0;
boolean upActive = false;
boolean downActive = false;

void setup() {
    // Set MIDI baud rate:
    Serial.begin(31250);
    // setup hi_hat switch
    pinMode(HI_HAT_SWITCH, INPUT);
    pinMode(MINUS_SWITCH, INPUT);
    pinMode(PLUS_SWITCH, INPUT);

    // //read last saved NOTES
    // for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
    //     temporaryValue = EEPROM.read(idx);
    //     CONF_MATRIX_SIZE[idx][IDX_NOTE] = temporaryValue;
    // }

}

void loop() {
    checkButtons();
    
    // if (digitalRead(MINUS_SWITCH) == HIGH) {
    //     if (time == 0) time = millis();
    //     else {
    //         if (time )
    //     }
    // } else {
    //     time = 0;
    // }
    //scan arbitrary instruments
    for (int analogInput = 0; analogInput < ANALOG_INPUTS_SIZE; analogInput++) {
        calcVelocity = detectKnock(analogInput);
        if (calcVelocity > 0) {
            noteOn(CONF_MATRIX[analogInput][IDX_NOTE], calcVelocity);
        }
    }
    handleHiHat();
}

void checkButtons() {
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
    if (currentMode == 'P') {
        //TODO adicionar um a nota corrente
    }
}

void doubleLongPress() {

}

void downLongPress() {

}

void downShortPress() {

}

void handleHiHat() {
    //handle hi-hat
    calcVelocity = detectKnock(IDX_HI_HAT_OPENED);
    if (calcVelocity > 0) {
        switchState = digitalRead(HI_HAT_SWITCH);
        if (switchState == HIGH) {
            noteOn(CONF_MATRIX[IDX_HI_HAT_CLOSED][IDX_NOTE], calcVelocity);
        } else {
            noteOn(CONF_MATRIX[IDX_HI_HAT_OPENED][IDX_NOTE], calcVelocity);
        }
    }
    
    if (lastHiHatPosition != switchState) {
        noteOn(CONF_MATRIX[IDX_HI_HAT_PEDAL][IDX_NOTE], MAX_MIDI_VELOCITY); //no velocity
    }

    lastHiHatPosition = switchState;
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
