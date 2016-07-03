/*
  Mains Brightness Control
  Copyright (c) 2016 Subhajit Das

  Licence Disclaimer:

   This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <USB_MP3_Player_Remote.h>

#include <IRremote.h>        // necessary for remote data decoding
//#include <FRONTECH_TV_Remote.h>  // header file in which remote controller buttons are defined

const byte MIN_DELAY = 0;   // minimum triggerDelay ( possible value at least 0 (lag in ms) )
const byte MAX_DELAY = 8; // maximum triggerDelay ( possible value upto 8 (lag in ms) )
const byte outputCount = 3;   // number of outputs

/*
   Working - tested
  :: Variables ::
   output[] , triggerDelay[] , onStatus[] , indicator[] used together to define their respective value.
   They represent different properties of same output. Their index number must be same when accessing them.
   output[] contains the output pin numbers on the microcontroller / arduino .
   triggerDelay[] contains the delay time of the output.
   onStatus[] represents their on status, 1 for on and 0 for off. Initially 0, as the all off.
   indicator[] contains the pin numbers of indicator LEDs, with respect to the outputs.
   InterruptFlag indicates the zero crossing.
   setIndicator() sets the output indicators in front.
  :: Functions ::
    void setup(), does initial setup (compulsory).
    void loop(), loop through iterations.
    setIndicator() sets the output indicators in front.
    trigger(), is the interrupt service routine that sets the InterruptFlag.
  User selects the output using remote. The data is received by IR receiver.
  Then it is decoded by decoder library. Based on received data, output is selected.
  Volume buttons set the brightness.
*/

byte output[outputCount] = {      // output pin no which's triggerDelay is adjusted
  13, A1, A3
};

byte triggerDelay [outputCount] = { // LED triggerDelay level
  MIN_DELAY, MIN_DELAY, MIN_DELAY
};

boolean onStatus[outputCount] = {   // states if LED or output is ON (true) or OFF (false)
  false, false, false
};

byte indicator[outputCount] = {   // selects idicator LED pins
  6, 7, 8
};

volatile byte selectedOutput = 0;  // used to select led to change triggerDelay

byte i; // used for iteration control in loops

const byte RECV_PIN = 5;  // recive pin for IR reciever input in arduino. can be any digital pin.
const byte INDICATOR_PIN = 10; //indicator for IR recv

IRrecv irrecv(RECV_PIN);  // calling constructor of IRrecv class, with receive pin as parameter
decode_results results;  // decoded input results are stored in results object

volatile boolean InterruptFlag = false;

boolean stateChanged = true;
long IndicationEndTime = 0;

void setup() {
  // defining all output pins as OUTPUT
  for (i = 0; i < outputCount; i++) {
    pinMode(output[i], OUTPUT);
    pinMode(indicator[i], OUTPUT);
  }
  pinMode(INDICATOR_PIN, OUTPUT);
  digitalWrite(INDICATOR_PIN, LOW);

  //Serial.begin(9600);  // starts serial communication, needed only for debugging
  irrecv.enableIRIn(); // Start the receiver

  // setting all indicators initially
  setIndicator();

  attachInterrupt(0, trigger, RISING);  // calls on every half cycle
}

void loop() {
  // turn indicator off after IndicationEndTime is crossed
  if (millis() >= IndicationEndTime) {
    digitalWrite(INDICATOR_PIN, LOW);
  }

  if (irrecv.decode(&results)) {
    stateChanged = true;

    long recv = results.value;  // taking received value againt pressed button in a variable
    //Serial.println(recv, HEX); // outputs the recieved value in serial monitor
    irrecv.resume(); // Receive the next value

    // performs specified job depending pressed button
    switch (recv) {
      /*
        NUM_1, NUM_2, NUM_2 ... are used to select the output directly.
        VOL is used to increase or decrease triggerDelay.
        CH is used to change LED selection serially, up or down.
        POWER button is used to toggle an output with MAX or MIN triggerDelay.
      */
      case NUM_1 : // executes when numreic 1 is pressed
        selectedOutput = 0;
        break;

      case NUM_2 : // executes when numreic 2 is pressed
        selectedOutput = 1;
        break;

      case NUM_3 : // executes when numreic 3 is pressed
        selectedOutput = 2;
        break;

      case NEXT : // executes when channel plus is pressed
        if (selectedOutput < outputCount - 1) // selects LED till  last LED
          selectedOutput ++;
        break;

      case PREVIOUS : // executes when channel minus is pressed
        if (selectedOutput > 0) // selects LED till  1st LED
          selectedOutput --;
        break;

      // lowers the triggerDelay of selected LED by DIF value
      case VOL_PLUS : // executes when button volume plus is pressed
        if (triggerDelay[selectedOutput] > MIN_DELAY)
          triggerDelay[selectedOutput]--;
        break;

      // highers the triggerDelay of selected LED by DIF value
      case VOL_MINUS : // executes when button volume minus is pressed
        if (triggerDelay[selectedOutput] < MAX_DELAY)
          triggerDelay[selectedOutput]++;
        break;

      // power button will turn all LED on or off depending on ledAny value
      case POWER :
        onStatus[selectedOutput] = !onStatus[selectedOutput];
        break;

      default:
        stateChanged = false;
        break;
    }
    if (stateChanged == true) {
      // set indicator to be on till IndicationEndTime is crossed
      digitalWrite(INDICATOR_PIN, HIGH);
      IndicationEndTime = millis() + 300;
    }
    setIndicator();  // turns on correct indicator LED
  }
  if (InterruptFlag == true) {
    byte i, count = 0, timePassed = 0;
    /*
       Variale 'count' records how many outputs are set to defined output
       Variale 'i' is for internal loop to loop through every output
    */
    for (count = 0; count < outputCount; ) { // a loop runs until all outputs are checked
      for (i = 0; i < outputCount; i++) { // checking every output if current time matches delay time
        if (triggerDelay[i] == timePassed) {
          count++;
          if (onStatus[i]) { // pulse is given only if output is light is on
            digitalWrite(output[i], HIGH);  // following three lines are for short pulse
            delayMicroseconds(10); // determines width of the pulse
            digitalWrite(output[i], LOW);
          }
        }
      }
      delay(1);
      timePassed++;
    }
    InterruptFlag = false;  // clearing the interrupt flag
  }
}

// turns on the indicator for selected led and disables others
void setIndicator() {
  for (i = 0; i < outputCount; i++) {
    if (i == selectedOutput)
      digitalWrite(indicator[i], HIGH);
    else
      digitalWrite(indicator[i], LOW);
  }
}

// triggered when an edge is recieved
void trigger() {
  InterruptFlag = true;
}
