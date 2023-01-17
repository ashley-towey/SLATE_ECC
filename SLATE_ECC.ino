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
#define NUMPIXELS 29  // define how many LEDs are in the strip
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
    Serial.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
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
  MP3player.setVolume(50, 50);

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
            Serial.print("pin ");
            Serial.print(i);
            Serial.println(" was just touched");
          }

          digitalWrite(LED_BUILTIN, HIGH);

          if (i <= 11 && i >= 0) {
            if (MP3player.isPlaying()) {
              // HAPPY electrode here, it looks so jolly!
              if (MPR121.isNewTouch(HAPPY_SWITCH_ELECTRODE)) {
                HAPPY_FUNCTION();
              }
              // EXCITED electrode here, it looks so pumped!  
              if (MPR121.isNewTouch(EXCITED_SWITCH_ELECTRODE)) {             
                EXCITED_FUNCTION();
              }
              // ANXIOUS electrode touched, it's fair enough really
              if (MPR121.isNewTouch(3)) {             
                ANXIOUS_FUNCTION();
              }

              if (lastPlayed == i && !REPLAY_MODE) {
                // if we're already playing the requested track, stop it
                // (but only if we're not in REPLAY_MODE)
                MP3player.stopTrack();

                if (!MPR121_DATASTREAM_ENABLE) {
                  Serial.print("stopping track ");
                  Serial.println(i-0);
                }
              } else {
                // if we're already playing a different track (or we're in
                // REPLAY_MODE), stop and play the newly requested one
                MP3player.stopTrack();
                MP3player.playTrack(i-0);

                if (!MPR121_DATASTREAM_ENABLE) {
                  Serial.print("playing track ");
                  Serial.println(i-0);
                }

                lastPlayed = i;
              }
            } else {
              // if we're playing nothing, play the requested track
              MP3player.playTrack(i-0);

              if (!MPR121_DATASTREAM_ENABLE) {
                Serial.print("playing track ");
                Serial.println(i-0);
              }

              lastPlayed = i;
            }
          }
      } else {
        if (MPR121.isNewRelease(i)) {
          if (!MPR121_DATASTREAM_ENABLE) {
            Serial.print("pin ");
            Serial.print(i);
            Serial.println(" is no longer being touched");
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
  Serial.println("HAPPY touched");
  if (PIXELS_ON) {  // if the LED strip is on
    PIXELS_ON = false;
    pixels.clear();

    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(0, 200, 0));
       pixels.show();
        delay(20);
      }

    pixels.show();
 } else {  // if the LED strip is off
    PIXELS_ON = true;

    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(0, 200, 0));
       pixels.show();
        delay(20);
      }
    }
}

void EXCITED_FUNCTION() {
  Serial.println("EXCITED touched");
  if (PIXELS_ON) {  // if the LED strip is on
    PIXELS_ON = false;
    pixels.clear();
    
    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(255, 255, 0));
       pixels.show();
        delay(20);
      }

    pixels.show();
 } else {  // if the LED strip is off
    PIXELS_ON = true;

    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(255, 255, 0));
       pixels.show();
        delay(20);
      }
    }
}

void ANXIOUS_FUNCTION() {
  Serial.println("ANXIOUS touched");
  if (PIXELS_ON) {  // if the LED strip is on
    PIXELS_ON = false;
    pixels.clear();

    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
       pixels.show();
        delay(20);
      }

   } else {  // if the LED strip is off
    PIXELS_ON = true;

    for (int i = 0; i < NUMPIXELS; i++) {  // for each LED within the strip
      pixels.setPixelColor(i, pixels.Color(255, 0, 0));
       pixels.show();
        delay(20);
      }
    }
}

