
/*MIDI CONSTANTS*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int MIDI_CMD_CC = 0xB0; 
/*Analog erros handling*/
const int ANALOG_DIRECT_LOW_LIMIT = 100;
const int ANALOG_DIRECT_HIGH_LIMIT = 1023;

/* BUFFER AND CONFIGURATION */
const int NOISE_DETECT_ITERATIONS = 30; 
const int CONF_MATRIX_SIZE = 16; 

//Pads                                CC     C1     C2     C3     KC     HH     RD     RB     T1     T3     T4     CB     AT     T2     SS     SN
const int     C_ANALOG_INPUT[16]   = {A0,    A1,    A2,    A3,    A4,    A5,    A6,    A7,    A8,    A9,    A10,   A11,   A12,   A13,   A14,   A15  };
const boolean C_ENABLED[16]        = {true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
const int     C_NOTE[16]           = {4,     77,    79,    81,    36,    49,    60,    61,    71,    67,    65,    47,    44,    69,    42,   38    };
const boolean C_CONTROL_CHANGE[16] = {true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
const int     C_SCAN_TIME[16]      = {7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7    };
//DECAY = LOWER more sensible HIGHER less sensible
const int     C_DECAY_TIME[16]     = {300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   500,   280  };
const int     C_MASK_TIME[16]      = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };

int W_LAST_BUFFER[16]              = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
unsigned long W_DECAY_TERM[16]     = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
int W_DECAY_START[16]              = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
int W_KNOCK_THRESHOLD[16]          = {1,     100,   100,   1,     1,     1,     1,     1,     1,     1,     1,     100,   100,   100,   80,   20    };
unsigned long W_SCANNING[16]       = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
unsigned long W_MASK_MILLIS[16]    = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
int W_CC_MIN[16]                   = {-1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1   };
int W_CC_MAX[16]                   = {-1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1   };

const int C_CHOKE_MATRIX_SIZE      = 3; 
const int C_CHOKE_PIN[3]           = {1,     2,     3};
const int C_CHOKE_NOTE[3]          = {101,   102,   103};
int       W_CHOKE_LAST_STATE[3]    = {LOW,   LOW,   LOW}; //normally opened

/* BUFFER AND CONFIGURATION */

const boolean DEBUG = false;

/*LED Setup*/
const int LED_PIN = LED_BUILTIN; //D13
int notePlaying = 0;
const boolean LED_ON_NOTE_ON = true;
/*LED Setup*/

void setup() {
    if (DEBUG) {
        Serial.begin(9600);
    } else {
        Serial.begin(31250);
    }

    /*captures the maximum analog read to each*/
    for (int i = 0; i < NOISE_DETECT_ITERATIONS; i++) {
        for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
            if (C_ENABLED[idx] && !C_CONTROL_CHANGE[idx]) {
                int sensorReading = analogRead(C_ANALOG_INPUT[idx]);
                int current = W_KNOCK_THRESHOLD[idx];
                if (current < sensorReading) {
                    W_KNOCK_THRESHOLD[idx] = sensorReading+1;
                }
            }
        }
    }

    /*DEFINE CHOKE BUTTON PIN MODE*/
    for (int idx = 0; idx < C_CHOKE_MATRIX_SIZE; idx++) {
        pinMode(C_CHOKE_PIN[idx], INPUT);
    }

    if (DEBUG) {
        Serial.println("Startup W_KNOCK_THRESHOLD");
        for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
            if (C_ENABLED[idx]) {
                Serial.print(idx);
                Serial.print(" > ");
                Serial.println(W_KNOCK_THRESHOLD[idx]);
            }
        }
    }

    pinMode(LED_PIN, OUTPUT);
}

void loop() {
    handlePlayMode();
    handleChoke();
}

void handlePlayMode() {
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        if (C_ENABLED[idx]) {
            if (C_CONTROL_CHANGE[idx]) {
                dealWithControlChange(idx);
            } else {
                detectKnock(idx);
            }
        }
    }
    
}

void handleChoke() {
    for (int idx = 0; idx < C_CHOKE_MATRIX_SIZE; idx++) {
        int chokeState = digitalRead(C_CHOKE_PIN[idx]);
        if (chokeState != W_CHOKE_LAST_STATE[idx] && chokeState == HIGH) {
            midiNoteOn(C_CHOKE_NOTE[idx], 127);
        }
        W_CHOKE_LAST_STATE[idx] = chokeState;
    }
}

void dealWithControlChange(int idx) {
    int analogInput = C_ANALOG_INPUT[idx];
    int sensorReading = analogRead(analogInput);

    /*
        adjust max and min
        this adjust due to all hardware and environment variables around the hihat sensor and other types of sensors
        environment lights, circuit resistors, etc...
    */
    if (W_CC_MIN[idx] == -1 || sensorReading < W_CC_MIN[idx]) {
        W_CC_MIN[idx] = sensorReading;
    }
    if (W_CC_MAX[idx] == -1 || sensorReading > W_CC_MAX[idx]) {
        W_CC_MAX[idx] = sensorReading;
    }

    int range = W_CC_MAX[idx] - W_CC_MIN[idx];
    int value = sensorReading - W_CC_MIN[idx];
    int converted = int((float(value) * 127.0) / float(range));

    int lastConvertedValue = W_LAST_BUFFER[idx];

    W_LAST_BUFFER[idx] = converted; // store last reading 

    if (lastConvertedValue != converted) {
        int control = C_NOTE[idx];
        midiControlChange(control, value);
    }
}

void sendNoteOn(int idx, int highResVelocity) {
    if (millis() - W_MASK_MILLIS[idx] > C_MASK_TIME[idx] ) {
        W_MASK_MILLIS[idx] = 0;
    }

    int note = C_NOTE[idx];
    if (W_MASK_MILLIS[idx] == 0) {
        int velocity = int ((float(highResVelocity)/1023.0)*127.0); 
        midiNoteOn(note, velocity);
        notePlaying++;
        checkLed();

        W_DECAY_TERM[idx] = millis() + C_DECAY_TIME[idx];
        W_MASK_MILLIS[idx] = millis();
        W_DECAY_START[idx] = highResVelocity;
    } else if (DEBUG){
        Serial.print("MASK TIME OBFUSCOU: ");
        Serial.println(note);
    }

}


void sendNoteOff(int idx) {
    int note = C_NOTE[idx];
    midiNoteOff(note);
    notePlaying--;
    checkLed();
}

void checkLed() {
    if (LED_ON_NOTE_ON) {
        if (notePlaying > 0) {
            digitalWrite(LED_PIN, HIGH); 
        } else {
            digitalWrite(LED_PIN, LOW); 
        }
    }
}

void midiNoteOn(int pitch, int velocity) {
    if (DEBUG) {
        Serial.print("NOTE ON ");
        Serial.print(pitch);
        Serial.print(" = ");
        Serial.println(velocity);
    } else {
        Serial.write(MIDI_CMD_NOTE_ON);
        Serial.write(pitch);
        Serial.write(velocity);
    }
}
void midiNoteOff(int pitch) {
    if (DEBUG) {
        Serial.print("NOTE OFF ");
        Serial.println(pitch);
    } else {
        Serial.write(MIDI_CMD_NOTE_ON);
        Serial.write(pitch);
        Serial.write(0);
    }
}
void midiControlChange(int cc, int value) {
    if (DEBUG) {
        Serial.print("CC ");
        Serial.print(cc);
        Serial.print(" = ");
        Serial.println(value);
    } else {
        Serial.write(MIDI_CMD_CC);
        Serial.write(cc);
        Serial.write(value);
    }
}

int filteredPiezoReading(int analogInputIdx) {
    int sensorReading = analogRead(C_ANALOG_INPUT[analogInputIdx]);
    if (sensorReading >= W_KNOCK_THRESHOLD[analogInputIdx] ) {
        return sensorReading;
    }
    return 0;
}

void decayLastBuffer(int idx) {
    if (W_LAST_BUFFER[idx] > 0) {
        unsigned long millisToCome = W_DECAY_TERM[idx] - millis();
        if (millisToCome <= 0) {
            W_LAST_BUFFER[idx] = 0;
            W_DECAY_TERM[idx] = 0;
            W_DECAY_START[idx] = 0;
            sendNoteOff(idx);
        } else {
            float percentage = float(millisToCome)/(float(C_DECAY_TIME[idx]));
            int goal = W_DECAY_START[idx] * (1.0-percentage);
            if (goal < W_LAST_BUFFER[idx]) {
                W_LAST_BUFFER[idx] = goal;
            }
            if (goal == 0) {
                W_LAST_BUFFER[idx] = 0;
                W_DECAY_TERM[idx] = 0;
                W_DECAY_START[idx] = 0;
                sendNoteOff(idx);
            }
        }
    }
}

void detectKnock(int analogInputIdx) {
    int sensorReading = filteredPiezoReading(analogInputIdx);
    unsigned int now = millis();

    if (W_SCANNING[analogInputIdx] == 0) {
        if (sensorReading > 0 && sensorReading > W_LAST_BUFFER[analogInputIdx]) {
            W_SCANNING[analogInputIdx] = now+C_SCAN_TIME[analogInputIdx];
            W_LAST_BUFFER[analogInputIdx] = sensorReading;
        } else {
            decayLastBuffer(analogInputIdx);
        }
    } else if (now < W_SCANNING[analogInputIdx]) {
        if (sensorReading > W_LAST_BUFFER[analogInputIdx]) W_LAST_BUFFER[analogInputIdx] = sensorReading;            
    } else if (now >= W_SCANNING[analogInputIdx]) {
        sendNoteOn(analogInputIdx, W_LAST_BUFFER[analogInputIdx]);
        W_SCANNING[analogInputIdx] = 0;
    }
}

int analogReading(int analogInputIdx) {
    int analogInput = C_ANALOG_INPUT[analogInputIdx];
    int sensorReading = analogRead(analogInput);
    int lastReading = W_LAST_BUFFER[analogInputIdx];

    W_LAST_BUFFER[analogInputIdx] = sensorReading; // store last reading 
    if (sensorReading >= W_KNOCK_THRESHOLD[analogInputIdx] && sensorReading <= ANALOG_DIRECT_HIGH_LIMIT) {
        if (lastReading != sensorReading) {
            return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
        } else {
            return -1;
        }
    } else {
        return -1;
    }

}
