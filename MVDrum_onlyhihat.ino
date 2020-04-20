
/*MIDI CONSTANTS*/
const int MIDI_CMD_NOTE_ON = 0x90; //note on channel 01
const int MIDI_CMD_CC = 0xB0; 
/*Analog erros handling*/
const int KNOCK_THRESHOLD = 100;  //sensor range 0 to 1023
const int ANALOG_DIRECT_LOW_LIMIT = 100;
const int ANALOG_DIRECT_HIGH_LIMIT = 1023;

/* BUFFER AND CONFIGURATION */
const int CONF_MATRIX_SIZE = 16; 

const int IDX_ANALOG_INPUT    = 0;
const int IDX_ENABLED         = 1;
const int IDX_NOTE            = 2;
const int IDX_CONTROL_CHANGE  = 3;
const int IDX_MAX_CYCLES      = 4;
int CONF_MATRIX [16][5] {
/*  {ANALOG_INPUT, ENABLED , NOTE, CONTROL_CHANGE, MAX_CYCLES */
    {A0          , 1       , 4   , 1             , 10         },
    {A1          , 1       , 105 , 0             , 10         },
    {A2          , 1       , 106 , 0             , 10         },
    {A3          , 1       , 107 , 0             , 10         },
    {A4          , 1       , 108 , 0             , 10         },
    {A5          , 1       , 109 , 0             , 10         },
    {A6          , 1       , 110 , 0             , 10         },
    {A7          , 1       , 111 , 0             , 10         },
    {A8          , 1       , 112 , 0             , 10         },
    {A9          , 1       , 113 , 0             , 10         },
    {A10         , 1       , 114 , 0             , 10         },
    {A11         , 1       , 115 , 0             , 10         },
    {A12         , 1       , 116 , 0             , 10         },
    {A13         , 1       , 117 , 0             , 10         },
    {A14         , 1       , 118 , 0             , 10         },
    {A15         , 1       , 119 , 0             , 10         }
};
const int IDX_LAST_BUFFER     = 0;
const int IDX_CURRENT_CYCLE   = 1;
const int IDX_KNOCK_THRESHOLD = 2;

int WORK_MATRIX[16][3] {
/*  { LAST_BUFFER      , CURRENT_CYCLE, KNOCK_THRESHOLD */
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 },
    { 0                , 0            , 0 }
};

int getAnalogByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_ANALOG_INPUT];
}

boolean isEnabledByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_ENABLED] == 1;
}

int getNoteByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_NOTE];
}

boolean isControlChange(int idx) {
    int cc = CONF_MATRIX[idx][IDX_CONTROL_CHANGE];
    return cc == 1;
}

int getMaxCyclesByIndex(int idx) {
    return CONF_MATRIX[idx][IDX_MAX_CYCLES];
}


int getLastBufferByIndex(int idx) {
    return WORK_MATRIX[idx][IDX_LAST_BUFFER];
}

void setLastBufferByIndex(int idx, int value) {
    WORK_MATRIX[idx][IDX_LAST_BUFFER] = value;
}

void setCycleByIndex(int idx, int value) {
    WORK_MATRIX[idx][IDX_CURRENT_CYCLE] = value;
}

int getCycleByIndex(int idx) {
    return WORK_MATRIX[idx][IDX_CURRENT_CYCLE];
}

int getKnockThresholdByIndex(int idx) {
    return WORK_MATRIX[idx][IDX_KNOCK_THRESHOLD];
}

void setKnockThresholdByIndex(int idx, int value) {
    WORK_MATRIX[idx][IDX_KNOCK_THRESHOLD] = value;
}

/* BUFFER AND CONFIGURATION */

void setup() {
    Serial.begin(31250);

    /*captures the maximum analog read to each*/
    int interactions = 0;
    for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
        if (getMaxCyclesByIndex(idx) > interactions) interactions = getMaxCyclesByIndex(idx);
    }
    interactions = interactions * 3;

    for (int i = 0; i < interactions; i++) {
        for (int idx = 0; idx < CONF_MATRIX_SIZE; idx++) {
            if (isEnabledByIndex(idx)) {
                int sensorReading = analogRead(getAnalogByIndex(idx));
                int current = getKnockThresholdByIndex(idx);
                if (current < sensorReading) {
                    setKnockThresholdByIndex(idx, sensorReading+1);
                }
            }
        }
    }
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
            int currentCycle = getCycleByIndex(idx);
            if (currentCycle == 0) {
                if (value > 0) {
                    sendNoteOn(idx, value);
                    setCycleByIndex(idx, getMaxCyclesByIndex(idx));
                } 
            } else {
                currentCycle--;
                setCycleByIndex(idx, currentCycle);
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
