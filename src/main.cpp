#include <SoftwareSerial.h>
#include "WProgram.h"
#include <Arduino.h>
#include <Audio.h>
#include "effect_frequencyshifter.h"
#include "BALibrary.h"

using namespace BALibrary;

BAAudioControlWM8731 codecControl;

AudioFrequencyShifter fS;   
AudioInputI2S           adc;
AudioOutputI2S           dac;       
AudioConnection          patchChord1(adc, 0, fS, 0);
AudioConnection          patchCord1(fS, 0, dac, 0);
AudioConnection          patchCord2(fS, 0, dac, 1);
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, 0, BA_EXPAND_NUM_LED);

extern "C" int main() {


  TGA_PRO_REVB();
  
  pinMode(BA_EXPAND_LED1_PIN, OUTPUT);

  digitalWrite(BA_EXPAND_LED1_PIN, HIGH);
  delay(100);
  digitalWrite(BA_EXPAND_LED1_PIN, LOW);



  fS.frequency(100);
  fS.amplitude(0.5);

  delay(5);
  AudioMemory(48);
  codecControl.enable();
  codecControl.setDacMute(false);
  codecControl.setHeadphoneVolume(0.5f);

  controls.addPot(BA_EXPAND_POT1_PIN, 0, 1020, true);
  controls.addPot(BA_EXPAND_POT2_PIN, 0, 1020, true);
  controls.addPot(BA_EXPAND_POT3_PIN, 0, 1020, true);

  while (1) {


    float value;
    if (controls.checkPotValue(0, value)) {
      fS.frequency(static_cast<float>(value)*1000.0);
    }
    
    if (controls.checkPotValue(1, value)) {
      for (int i = 0; i<8; ++i) {
        fS.amplitude(static_cast<float>(value));
      }
    }
    
    if (controls.checkPotValue(2, value)) {
  codecControl.setHeadphoneVolume(static_cast<float>(value));
    }

  }
  
}
