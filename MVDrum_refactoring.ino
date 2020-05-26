
/*MIDI CONSTANTS*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int MIDI_CMD_CC = 0xB0; 
/*Analog erros handling*/
const int ANALOG_DIRECT_LOW_LIMIT = 100;
const int ANALOG_DIRECT_HIGH_LIMIT = 1023;

/* BUFFER AND CONFIGURATION */
const int NOISE_DETECT_ITERATIONS = 30; 
const int CONF_MATRIX_SIZE = 16; 

const int     C_ANALOG_INPUT[16]   = {A0  ,  A1,    A2,    A3,    A4,    A5,    A6,    A7,    A8,    A9,    A10,   A11,   A12,   A13,   A14,   A15  };
const boolean C_ENABLED[16]        = {false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true};
const int     C_NOTE[16]           = {4,     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,   115,   116,   117,   118,   119  };
const boolean C_CONTROL_CHANGE[16] = {true,  false, false, false, false, false, false, false, false, false, false, false, false, false, false, false};
//DECAY = LOWER more sensible HIGHER less sensible
const int     C_SCAN_TIME[16]      = {7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7,     7    };
const int     C_DECAY_TIME[16]     = {300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300,   300  };

int W_LAST_BUFFER[16]              = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
unsigned long W_DECAY_TERM[16]     = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
int W_DECAY_START[16]              = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
int W_KNOCK_THRESHOLD[16]          = {75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75,    75   };
unsigned long W_SCANNING[16]       = {0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0,     0    };
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
}

void handlePlayMode() {
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        if (C_ENABLED[idx]) {
            if (C_CONTROL_CHANGE[idx]) {
                int value = analogReading(idx);
                if (value != -1) {
                    sendControlChange(idx, value);
                }
            } else {
                detectKnock(idx);
            }
        }
    }

}

void sendNoteOn(int idx, int highResVelocity) {
    int note = C_NOTE[idx];
    int velocity = int ((float(highResVelocity)/1023.0)*127.0); 
    midiNoteOn(note, velocity);
    notePlaying++;
    checkLed();

    W_DECAY_TERM[idx] = millis() + C_DECAY_TIME[idx];
    W_DECAY_START[idx] = highResVelocity;
}

void sendControlChange(int idx, int value) {
    int control = C_NOTE[idx];
    midiControlChange(control, value);
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
