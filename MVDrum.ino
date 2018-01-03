/*
Parameters
*/
const int MIDI_CMD_NOTE_ON = 0x90;
const int KNOCK_THRESHOLD = 100;  
const int MAX_MIDI_VELOCITY = 127;

/*Drum note Map*/
const int ACOUSTIC_BASS_DRUM = 35;  
const int BASS_DRUM_1        = 36;  
const int SIDE_STICK         = 37;  
const int ACOUSTIC_SNARE     = 38;  
const int HAND_CLAP          = 39;  
const int ELECTRIC_SNARE     = 40;  
const int LOW_FLOOR_TOM      = 41;  
const int CLOSED_HI_HAT      = 42;  
const int HIGH_FLOOR_TOM     = 43;  
const int PEDAL_HI_HAT       = 44;  
const int LOW_TOM            = 45;  
const int OPEN_HI_HAT        = 46;  
const int LOW_MID_TOM        = 47;  
const int HI_MID_TOM         = 48;  
const int CRASH_CYMBAL_1     = 49;  
const int HIGH_TOM           = 50;  
const int RIDE_CYMBAL_1      = 51;  
const int CHINESE_CYMBAL     = 52;  
const int RIDE_BELL          = 53;  
const int TAMBOURINE         = 54;  
const int SPLASH_CYMBAL      = 55;  
const int COWBELL            = 56;  
const int CRASH_CYMBAL_2     = 57;  
const int VIBRASLAP          = 58;  
const int RIDE_CYMBAL_2      = 59;  
const int HI_BONGO           = 60;  
const int LOW_BONGO          = 61;  
const int MUTE_HI_CONGA      = 62;  
const int OPEN_HI_CONGA      = 63;  
const int LOW_CONGA          = 64;  
const int HIGH_TIMBALE       = 65;  
const int LOW_TIMBALE        = 66;  
const int HIGH_AGOGO         = 67;  
const int LOW_AGOGO          = 68;  
const int CABASA             = 69;  
const int MARACAS            = 70;  
const int SHORT_WHISTLE      = 71;  
const int LONG_WHISTLE       = 72;  
const int SHORT_GUIRO        = 73;  
const int LONG_GUIRO         = 74;  
const int CLAVES             = 75;  
const int HI_WOOD_BLOCK      = 76;  
const int LOW_WOOD_BLOCK     = 77;  
const int MUTE_CUICA         = 78;  
const int OPEN_CUICA         = 79;  
const int MUTE_TRIANGLE      = 80;  
const int OPEN_TRIANGLE      = 81;

const int IDX_ANALOG_INPUT = 0;
const int IDX_NOTE = 1;
/*Array to define witch note associated with each drum note*/
const int NOTE_SENSOR_MATRIX [8][2] {
    //{IDX_ANALOG_INPUT, IDX_NOTE}
    {A0,ACOUSTIC_BASS_DRUM}, //<----- CHANGE HERE
    {A1,ACOUSTIC_SNARE}    , //<----- CHANGE HERE 
    {A2,ACOUSTIC_SNARE}    , //<----- CHANGE HERE 
    {A3,HIGH_TOM}          , //<----- CHANGE HERE 
    {A4,CRASH_CYMBAL_1}    , //<----- CHANGE HERE 
    {A5,RIDE_CYMBAL_2}     , //<----- CHANGE HERE 
    {A6,CRASH_CYMBAL_2}    , //<----- CHANGE HERE 
    {A7,OPEN_HI_HAT} //reserved
};
const int ANALOG_INPUTS_SIZE = 7; //not run last analog input
const int HI_HAT_SWITCH = 2;
const int HI_HAT_ANALOG_INPUT = A7;
const int NOTE_HI_HAT_OPENED = OPEN_HI_HAT;   //<----- CHANGE HERE 
const int NOTE_HI_HAT_CLOSED = CLOSED_HI_HAT; //<----- CHANGE HERE 
const int NOTE_HI_HAT_PEDAL = PEDAL_HI_HAT;   //<----- CHANGE HERE 

/*Variables*/
int sensorReading = 0;   
int calcVelocity = 0;
int switchState = 0;
int lastHiHatPosition = LOW;
int lastKnockBuffer[] {0,0,0,0,0,0,0,0};

void setup() {
    // Set MIDI baud rate:
    Serial.begin(31250);
    // setup hi_hat switch
    pinMode(HI_HAT_SWITCH, INPUT);
}

void loop() {
    //scan arbitrary instruments
    for (int analogInput = 0; analogInput < ANALOG_INPUTS_SIZE; analogInput++) {
        calcVelocity = detectKnock(analogInput);
        if (calcVelocity > 0) {
            noteOn(NOTE_SENSOR_MATRIX[analogInput][IDX_NOTE], calcVelocity);
        }
    }
    handleHiHat();
}

void handleHiHat() {
    //handle hi-hat
    calcVelocity = detectKnock(7);
    if (calcVelocity > 0) {
        switchState = digitalRead(HI_HAT_SWITCH);
        if (switchState == HIGH) {
            noteOn(NOTE_HI_HAT_CLOSED, calcVelocity);
        } else {
            noteOn(NOTE_HI_HAT_OPENED, calcVelocity);
        }
    }
    
    if (lastHiHatPosition != switchState) {
        noteOn(NOTE_HI_HAT_PEDAL, MAX_MIDI_VELOCITY); //no velocity
    }

    lastHiHatPosition = switchState;
}

void noteOn(int pitch, int velocity) {
  Serial.write(MIDI_CMD_NOTE_ON);
  Serial.write(pitch);
  Serial.write(velocity);
}

int detectKnock(int analogInputIdx) {
    sensorReading = analogRead(NOTE_SENSOR_MATRIX[analogInputIdx][IDX_ANALOG_INPUT]);
    if (sensorReading >= KNOCK_THRESHOLD && lastKnockBuffer[analogInputIdx] < sensorReading) {
        lastKnockBuffer[analogInputIdx] = sensorReading; // store last reading for knock only if value is greater
        return int ((float(sensorReading)/1023.0)*127.0); //convert from 0 to 1023 range to 0 to 127 range
    } else {
        lastKnockBuffer[analogInputIdx] = sensorReading; // store last reading for knock only if value is greater
        return 0;
    }
}
