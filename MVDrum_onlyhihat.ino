
/*MIDI CONSTANTS*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int MIDI_CMD_CC = 0xB0; 
/*Analog erros handling*/
const int KNOCK_THRESHOLD = 100;  //sensor range 0 to 1023
const int ANALOG_DIRECT_LOW_LIMIT = 100;
const int ANALOG_DIRECT_HIGH_LIMIT = 1023;

/* BUFFER AND CONFIGURATION */
const int IDX_ANALOG_INPUT   = 0;
const int IDX_LAST_BUFFER    = 1;
const int IDX_NOTE           = 2;
const int IDX_CONTROL_CHANGE = 3;
const int CONF_MATRIX_SIZE = 16; 
int CONF_MATRIX [16][5] {
//  {IDX_ANALOG_INPUT, IDX_LAST_BUFFER      , IDX_NOTE, IDX_CONTROL_CHANGE
    {A0              , 0                    , 4       , 1                  },
    {A1              , 0                    , 105     , 0                  },
    {A2              , 0                    , 106     , 0                  },
    {A3              , 0                    , 107     , 0                  },
    {A4              , 0                    , 108     , 0                  },
    {A5              , 0                    , 109     , 0                  },
    {A6              , 0                    , 110     , 0                  },
    {A7              , 0                    , 111     , 0                  },
    {A8              , 0                    , 112     , 0                  },
    {A9              , 0                    , 113     , 0                  },
    {A10             , 0                    , 114     , 0                  },
    {A11             , 0                    , 115     , 0                  },
    {A12             , 0                    , 116     , 0                  },
    {A13             , 0                    , 117     , 0                  },
    {A14             , 0                    , 118     , 0                  },
    {A15             , 0                    , 119     , 0                  }
};


int getAnalogByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_ANALOG_INPUT];
}

int getNoteByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_NOTE];
}

int getLastBufferByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_LAST_BUFFER];
}

int setLastBufferByIndex(int idx, int value) {
    return CONF_MATRIX[idx][IDX_LAST_BUFFER] = value;
}

boolean isControlChange(int idx) {
    int cc = CONF_MATRIX[idx][IDX_CONTROL_CHANGE];
    return cc == 1;
}

/* BUFFER AND CONFIGURATION */

void setup() {
    Serial.begin(31250);
}

void loop() {
    handlePlayMode();
}

void handlePlayMode() {
    int value = 0;
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        if (isControlChange(idx)) {
            value = analogReading(idx);
            if (value != -1) {
                sendControlChange(idx, value);
            }
        } else {
            value = detectKnock(idx);
            if (value > 0) {
                sendNoteOn(idx, value);
            } 
        }
    }

}

void sendNoteOn(int idx, int velocity) {
    int note = getNoteByIndex(idx);
    midiNoteOn(note, velocity);
}
void sendControlChange(int idx, int value) {
    int control = getNoteByIndex(idx);
    midiControlChange(control, value);
}

void midiNoteOn(int pitch, int velocity) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(velocity);
}
void midiNoteOff(int pitch) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(0);
}
void midiControlChange(int cc, int value) {
    Serial.write(MIDI_CMD_CC);
    Serial.write(cc);
    Serial.write(value);
}

int readSensor(int analogInputIdx) {
    int analogInput = getAnalogByIndex(analogInputIdx);
    int sensorReading = analogRead(analogInput);

}

int detectKnock(int analogInputIdx) {
    int analogInput = getAnalogByIndex(analogInputIdx);
    int sensorReading = analogRead(analogInput);
    int lastKnock = getLastBufferByIndex(analogInputIdx);

    setLastBufferByIndex(analogInputIdx, sensorReading); // store last reading for knock 
    if (sensorReading >= KNOCK_THRESHOLD && lastKnock < sensorReading) {
        return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
    } else {
        return 0;
    }
}
int analogReading(int analogInputIdx) {
    int analogInput = getAnalogByIndex(analogInputIdx);
    int sensorReading = analogRead(analogInput);
    int lastReading = getLastBufferByIndex(analogInputIdx);

    setLastBufferByIndex(analogInputIdx, sensorReading); // store last reading 
    if (sensorReading >= ANALOG_DIRECT_LOW_LIMIT && sensorReading <= ANALOG_DIRECT_HIGH_LIMIT) {
        if (lastReading != sensorReading) {
            return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
        } else {
            return -1;
        }
    } else {
        return -1;
    }

}
