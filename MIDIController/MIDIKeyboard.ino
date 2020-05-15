/*
  MIDI keyboard and controller

  This code was designed for the following specs:
  - 61 keys keyboard (not sensitive to key velocity), with a 6x11 matrix cable
  - 8x Potentiometers - B10k
  - Arduino Pro Micro ATMEGA32U4 - The same microcontroller of the Arduino Leonardo

  It's based on an old tutorial made by Evan Kale (https://www.instructables.com/id/Add-MIDI-port-to-Keyboard/) in 2015

  Differing from the original, this project in particular has also eight potentiometers, used as MIDI controllers

  - Guilherme Bortoto
  May of 2020
*/

/*
  MIDIUSB Library (Needs to be added to the Aduino IDE. Tools > Manage Libraries)
  - Used for sending MIDI signals through USB
*/
#include "MIDIUSB.h"

/*
  Number of rows (control cables) and columns (data cables) on the matrix cable
  - Each column represents a group of six keys
  - The group of rows shows which key from a group of keys are being pressed
  
  e.g. If the combination Row 0 - Column 2 (both starting from 0) is received as positive,
  it means that the first key of the third column is pressed 
*/
#define NUM_COLS 11
#define NUM_ROWS 6


/*
  Key velocity - 0 to 255
  - This value dictates the intensity of a key press (since the keyboard is not capable of identifying it)
  - The default value 127 is arbitrary
*/
#define KEY_VELOCITY 127

/*
  MIDI Channel
  - Used to 
  - Not particularly important
*/
#define MIDI_CHANNEL 0


/*
  Number of potentiometers
*/
#define NUM_POTS 8


/*
  Row input pins
  - "Which Aduino pin is connected to which control cable?"
*/
const int row1Pin = 9;
const int row2Pin = 8;
const int row3Pin = 7;
const int row4Pin = 6;
const int row5Pin = 5;
const int row6Pin = 4;

/*
  Shift register (74HC595) specific pins
  - "Which Aduino pin is connected to which of the shift register controls?"
*/
const int dataPin = 10;   // Transfers data to the shift register
const int latchPin = 16;  // Allow or block the flow of information on the shift register
const int clockPin = 14;  // Control the pulses of the shift register

/*
  Key states matrix
  - "Is the X key being pressed?"
  - Follows the same matrix pattern of the keyboard matrix cable
*/
boolean keyPressed[NUM_ROWS][NUM_COLS];

/*
  Key note matrix
  - "What key (following the MIDI standard of assigning number to keys)
  should I send when I receive a positive signal from row X, col Y?"
  - [Known bug] For some bizarre hardware reason, the sequence of inputs is slightly shifted.
  For this reason, this table is also slightly shifted, in order to compensate
  - Middle C = 60
*/
uint8_t currentKeyToMidiMap[NUM_ROWS][NUM_COLS] = {
    {36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96},
    {0, 37, 43, 49, 55, 61, 67, 73, 79, 85, 91},
    {0, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92},
    {0, 39, 45, 51, 57, 63, 69, 75, 81, 87, 93},
    {0, 40, 46, 52, 58, 64, 70, 76, 82, 88, 94},
    {0, 41, 47, 53, 59, 65, 71, 77, 83, 89, 95}
  };

/*
WIP

uint8_t defaultKeyToMidiMap[NUM_ROWS][NUM_COLS] = {
    {36, 42, 48, 54, 60, 66, 72, 78, 84, 90, 96},
    {0, 37, 43, 49, 55, 61, 67, 73, 79, 85, 91},
    {0, 38, 44, 50, 56, 62, 68, 74, 80, 86, 92},
    {0, 39, 45, 51, 57, 63, 69, 75, 81, 87, 93},
    {0, 40, 46, 52, 58, 64, 70, 76, 82, 88, 94},
    {0, 41, 47, 53, 59, 65, 71, 77, 83, 89, 95}
  };

void shiftOctaves(int shiftBy){

  // Default state
  if(shiftBy == 0){
    for(int i=0; i<NUM_ROWS; i++){
      for(int j=0; j<NUM_COLS; j++){
        currentKeyToMidiMap[i][j] = defaultKeyToMidiMap[i][j];
      }
    }
  }

  // 
}

*/

// Máscaras de bits para escaneamento
int bits[] =
{ 
  B11111110,
  B11111101,
  B11111011,
  B11110111,
  B11101111,
  B11011111,
  B10111111,
  B01111111
};

int potState[] = {
  0, 0, 0, 0, 0, 0, 0, 0
};

/*
int potBits[NUM_POTS][3] = {
  {LOW, LOW, LOW},
  {LOW, LOW, HIGH},
  {LOW, HIGH, LOW},
  {LOW, HIGH, HIGH},
  {HIGH, LOW, LOW},
  {HIGH, LOW, HIGH},
  {HIGH, HIGH, LOW},
  {HIGH, HIGH, HIGH}
};
*/

int potBits[NUM_POTS][3] = {
  {LOW, LOW, HIGH},
  {LOW, HIGH, HIGH},
  {HIGH, HIGH, HIGH},
  {HIGH, LOW, HIGH},
  {LOW, HIGH, LOW},
  {HIGH, LOW, LOW},
  {LOW, LOW, LOW},
  {HIGH, HIGH, LOW}
};


void setup(){
  Serial.begin(9600); // For debugging
 
 // keyPressed matrix initialization
 for(int colCtr = 0; colCtr < NUM_COLS; ++colCtr) {
  for(int rowCtr = 0; rowCtr < NUM_ROWS; ++rowCtr) {
    keyPressed[rowCtr][colCtr] = false;
  }
 }

 pinMode(dataPin, OUTPUT);
 pinMode(clockPin, OUTPUT);
 pinMode(latchPin, OUTPUT);
 
 pinMode(row1Pin, INPUT);
 pinMode(row2Pin, INPUT);
 pinMode(row3Pin, INPUT);
 pinMode(row4Pin, INPUT);
 pinMode(row5Pin, INPUT);
 pinMode(row6Pin, INPUT);

 pinMode(15, OUTPUT);
 pinMode(A0, INPUT);
 pinMode(A1, OUTPUT);
 pinMode(A2, OUTPUT);

}

void loop()
{
  
  // Reading potentiometers
  for(int i=0; i<NUM_POTS; i++){
    digitalWrite(15, potBits[i][0]);
    digitalWrite(A1, potBits[i][1]);
    digitalWrite(A2, potBits[i][2]);

    int sensorValue = analogRead(A0);

    if(abs(sensorValue - potState[i]) > 25){
      potState[i] = sensorValue;
      potChange(i, sensorValue);
    }
  }
  

  //delay(1000);
  //printMap();
  for (int colCtr = 0; colCtr < NUM_COLS; ++colCtr)
  {
    //scan next column
    scanColumn(colCtr);
    
    //get row values at this column
    int rowValue[NUM_ROWS];
    
    rowValue[0] = !digitalRead(row1Pin);
    rowValue[1] = !digitalRead(row2Pin);
    rowValue[2] = !digitalRead(row3Pin);
    rowValue[3] = !digitalRead(row4Pin);
    rowValue[4] = !digitalRead(row5Pin);
    rowValue[5] = !digitalRead(row6Pin);
        
    // process keys pressed
    for(int rowCtr=0; rowCtr<NUM_ROWS; ++rowCtr) {
      if(rowValue[rowCtr] != 0 && !keyPressed[rowCtr][colCtr]) {
        keyPressed[rowCtr][colCtr] = true;
        noteOn(MIDI_CHANNEL, currentKeyToMidiMap[rowCtr][colCtr], KEY_VELOCITY);
      }
    }

    // process keys released
    for(int rowCtr=0; rowCtr<NUM_ROWS; ++rowCtr) {
      if(rowValue[rowCtr] == 0 && keyPressed[rowCtr][colCtr]) {
        keyPressed[rowCtr][colCtr] = false;
        noteOff(MIDI_CHANNEL, currentKeyToMidiMap[rowCtr][colCtr], KEY_VELOCITY);
      }
    }

    /*
    Serial.println(
      "Column " + String(colCtr)
      + ": " + String(rowValue[0])
      + " " + String(rowValue[1])
      + " " + String(rowValue[2])
      + " " + String(rowValue[3])
      + " " + String(rowValue[4])
      + " " + String(rowValue[5])
      + "\n");
    */
  }
  

  
}

void scanColumn(int colNum){
  digitalWrite(latchPin, LOW);
  if(0 <= colNum && colNum <= 7){
    shiftOut(dataPin, clockPin, MSBFIRST, B11111111); //right sr
    shiftOut(dataPin, clockPin, MSBFIRST, bits[colNum]); //left sr
  } else {
    shiftOut(dataPin, clockPin, MSBFIRST, bits[colNum-8]); //right sr
    shiftOut(dataPin, clockPin, MSBFIRST, B11111111); //left sr
  }
  digitalWrite(latchPin, HIGH);
}

/*
void printMap(){
 for(int colCtr = 0; colCtr < NUM_COLS; ++colCtr) {
  for(int rowCtr = 0; rowCtr < NUM_ROWS; ++rowCtr) {
    Serial.println("currentKeyToMidiMap[" + String(rowCtr) + "][" + String(colCtr) + "]: " + String(currentKeyToMidiMap[rowCtr][colCtr]));
  }
 }
}
*/

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
  MidiUSB.flush();
  //Serial.println("[Note ON] Channel: " + String(channel) + " | Pitch: " + String(pitch) + " | Velocity: " + String(velocity));
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
  MidiUSB.flush();  
  //Serial.println("[Note OFF] Channel: " + String(channel) + " | Pitch: " + String(pitch) + " | Velocity: " + String(velocity));
}

void potChange(int potIndex, int value){
  value = 1024 - value;
  
  // Se o valor for muito próximo a zero, torne o valor zero
  if(value < 25){
    value = 0;
  } else {
    value = map(value, 0, 1024, 0, 127);
  }
  controlChange(MIDI_CHANNEL, potIndex, value);
}

void controlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, 0xB0 | channel, control, value};
  MidiUSB.sendMIDI(event);
  MidiUSB.flush();
  //Serial.println("[ControlChange] Channel: " + String(channel) + " | Control: " + String(control) + " | Value: " + String(value));
}
