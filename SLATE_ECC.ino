/******************************************************************

 SLATE ECC
 ---------

 Emotion Communication Cube [both a tool & a toy]
 Created by Ashley Towey for Physical Computing at CCI on 10th Jan 2023
 ---------

 Base code based on Bare Conductive example code [filename: Touch_MP3.ino]
 And merged with code from the neopixel example [filename: neopixel_touch_switch.ino]

******************************************************************/

// compiler error handling - linking to an external file
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>
#include <MPR121_Datastream.h>
#include <Wire.h>

// neopixel libraries [brought from neopixel example]
#include <Adafruit_NeoPixel.h>

// MP3 includes
#include <SPI.h>
#include <SdFat.h>
#include <FreeStack.h>
#include <SFEMP3Shield.h>

// touch constants
const uint32_t BAUD_RATE = 115200;
const uint8_t MPR121_ADDR = 0x5C;
const uint8_t MPR121_INT = 4;

// serial monitor behaviour constants
const bool WAIT_FOR_SERIAL = false;

// MPR121 datastream behaviour constants
const bool MPR121_DATASTREAM_ENABLE = false;

// MP3 variables
uint8_t result;
uint8_t lastPlayed = 0;

// MP3 constants
SFEMP3Shield MP3player;

// MP3 behaviour constants
const bool REPLAY_MODE = true;  // by default, touching an electrode repeatedly will
                                // play the track again from the start each time
                                //
                                // if you set this to false, repeatedly touching an
                                // electrode will stop the track if it is already
                                // playing, or play it from the start if it is not

// SD card instantiation
SdFat sd;

// Switch numbers for the different emotions and light controls further down
/***************************/
const uint8_t HAPPY_SWITCH_ELECTRODE = 0;
const uint8_t EXCITED_SWITCH_ELECTRODE = 1;
const uint8_t ANXIOUS_SWITCH_ELECTRODE = 3;
const uint8_t SAD_SWITCH_ELECTRODE = 2;

// led switch variable
bool PIXELS_ON = false;

// neopixle behaviour constants
#define OUTPUT_PIN 11 // pin on the bareconductive board that will be the neopixel data output
#define NUMPIXELS 10  // define how many LEDs are in the strip
Adafruit_NeoPixel pixels(NUMPIXELS, OUTPUT_PIN, NEO_GRB + NEO_KHZ800);
/**************************/

void setup() {
  Serial.begin(BAUD_RATE);
  pinMode(LED_BUILTIN, OUTPUT);

  // Remove this section of code when I plug into external power
  /*********************/
  if (WAIT_FOR_SERIAL) {
    while (!Serial);
  }
  /*********************/

  if (!sd.begin(SD_SEL, SPI_HALF_SPEED)) {
    sd.initErrorHalt();
  }

  if (!MPR121.begin(MPR121_ADDR)) {
    Serial.println(F("error setting up MPR121"));
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println(F("no error"));
        break;
      case ADDRESS_UNKNOWN:
        Serial.println(F("incorrect address"));
        break;
      case READBACK_FAIL:
        Serial.println(F("readback failure"));
        break;
      case OVERCURRENT_FLAG:
        Serial.println(F("overcurrent on REXT pin"));
        break;
      case OUT_OF_RANGE:
        Serial.println(F("electrode out of range"));
        break;
      case NOT_INITED:
        Serial.println(F("not initialised"));
        break;
      default:
        Serial.println(F("unknown error"));
        break;
    }
    while (1);
  }

  MPR121.setInterruptPin(MPR121_INT);

  if (MPR121_DATASTREAM_ENABLE) {
    MPR121.restoreSavedThresholds();
    MPR121_Datastream.begin(&Serial);
  } else {
    MPR121.setTouchThreshold(40);
    MPR121.setReleaseThreshold(20);
  }

  MPR121.setFFI(FFI_10);
  MPR121.setSFI(SFI_10);
  MPR121.setGlobalCDT(CDT_4US);  // reasonable for larger capacitances
  
  digitalWrite(LED_BUILTIN, HIGH);  // switch on user LED while auto calibrating electrodes
  delay(1000);
  MPR121.autoSetElectrodes();  // autoset all electrode settings
  digitalWrite(LED_BUILTIN, LOW);

  result = MP3player.begin();
  MP3player.setVolume(20, 20);

  if (result != 0) {
    Serial.print("Error code: ");
    Serial.print(result);
    Serial.println(" when trying to start MP3 player");
  }

  // Code to setup the output pin [brough across from neopixel example]
  pinMode(OUTPUT_PIN, OUTPUT);
  digitalWrite(OUTPUT_PIN, LOW);

  pixels.begin();
  pixels.clear();
  pixels.show();
  // end of neopixel code snippet
}

void loop() {
  MPR121.updateAll();
  // only make an action if we have one or fewer pins touched
  // ignore multiple touches
  if (MPR121.getNumTouches() <= 1) {
    for (int i=0; i < 12; i++) {  // check which electrodes were pressed
      if (MPR121.isNewTouch(i)) {
          if (!MPR121_DATASTREAM_ENABLE) {
            // Serial.print("pin ");
            // Serial.print(i);
            // Serial.println(" was just touched");
          }

          digitalWrite(LED_BUILTIN, HIGH);

          if (i <= 11 && i >= 0) {
              /**************************
              Move this block of code around to test
              ***************************/
              // If the corresponding electrodes are touched the lighting functions are triggered
              // Code only being triggered if another track is already playing
              // Durrrh it was in an if statement that only happened if something was playing
              // what a blooming idiot. 

              // HAPPY electrode here, it looks so jolly!
              if (MPR121.isNewTouch(HAPPY_SWITCH_ELECTRODE)) {
                HAPPY_FUNCTION();
              }
              // EXCITED electrode here, it looks so pumped!  
              if (MPR121.isNewTouch(EXCITED_SWITCH_ELECTRODE)) {             
                EXCITED_FUNCTION();
              }
              // ANXIOUS electrode touched, it's fair enough really
              if (MPR121.isNewTouch(ANXIOUS_SWITCH_ELECTRODE)) {             
                ANXIOUS_FUNCTION();
              }
              // SAD electrode touched here, oh that really is so sad! 
              if (MPR121.isNewTouch(SAD_SWITCH_ELECTRODE)) {
                SAD_FUNCTION();
              }
              if (MPR121.isNewTouch(6)) { // reset button to turn neopixels off
                pixels.clear();
                pixels.show();
                // Serial.println("Neopixel OFF"); // communication is key
              }
              /***************************************/

            if (MP3player.isPlaying()) {

              if (lastPlayed == i && !REPLAY_MODE) {
                // if we're already playing the requested track, stop it
                // (but only if we're not in REPLAY_MODE)
                MP3player.stopTrack();

                if (!MPR121_DATASTREAM_ENABLE) {
                  // Serial.print("stopping track ");
                  // Serial.println(i-0);
                }
              } else {
                // if we're already playing a different track (or we're in
                // REPLAY_MODE), stop and play the newly requested one
                MP3player.stopTrack();
                MP3player.playTrack(i-0);

                if (!MPR121_DATASTREAM_ENABLE) {
                  // Serial.print("playing track ");
                  // Serial.println(i-0);
                }

                lastPlayed = i;
              }
            } else {
              // if we're playing nothing, play the requested track
              MP3player.playTrack(i-0);

              if (!MPR121_DATASTREAM_ENABLE) {
                // Serial.print("playing track ");
                // Serial.println(i-0);
              }

              lastPlayed = i;
            }
          }
      } else {
        if (MPR121.isNewRelease(i)) {
          if (!MPR121_DATASTREAM_ENABLE) {
            // Serial.print("pin ");
            // Serial.print(i);
            // Serial.println(" is no longer being touched");
          }

          digitalWrite(LED_BUILTIN, LOW);
        }
      }
    }
  }

  if (MPR121_DATASTREAM_ENABLE) {
    MPR121_Datastream.update();
  }
}

void HAPPY_FUNCTION() {
  // Serial.println("HAPPY lights triggered");
  // Code brought in from simple neopixel light example
  // much simpler and now seems to be working in my code too! Yippee! 
  // Very happy! Coincidence? 

  pixels.clear(); // Set all pixel colors to 'off' 
  pixels.show(); // and clear pre existing values

  for(int i=0; i<NUMPIXELS; i++) { // run through all the neopixels
    pixels.setPixelColor(i, pixels.Color(0, 200, 0)); // green colour to signify the zone
  }
    pixels.show();   // Send the updated pixel colors to the hardware.
    delay(20); // Pause before moving on to the rest of the loop function
}

void EXCITED_FUNCTION() {
  // Serial.println("EXCITED lights triggered");
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();
  for(int i=0; i<NUMPIXELS; i++) { // run through all the pixels
    pixels.setPixelColor(i, pixels.Color(200, 200, 0)); // orange  colour here
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(20); // Pause before next pass through loop
}

void ANXIOUS_FUNCTION() {
  // Serial.println("ANXIOUS lights triggered");
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();
  for(int i=0; i<NUMPIXELS; i++) { // run through all the pixels
    pixels.setPixelColor(i, pixels.Color(200, 150, 0)); // orange  colour here
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(20); // Pause before next pass through loop
}

void SAD_FUNCTION() {
  // Serial.println("SAD lights triggered");
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();
  for(int i=0; i<NUMPIXELS; i++) { // run through all the pixels
    pixels.setPixelColor(i, pixels.Color(200, 0, 0)); // orange  colour here
  }
  pixels.show();   // Send the updated pixel colors to the hardware.
  delay(20); // Pause before next pass through loop
}
