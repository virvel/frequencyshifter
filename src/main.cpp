#include "WProgram.h"
#include <Arduino.h>
#include <OpenAudio_ArduinoLibrary.h>
#include "effect_frequencyshifter_f32.h"
#include "BALibrary.h"

using namespace BALibrary;

BAAudioControlWM8731 codecControl;

AudioFrequencyShifter_F32 fS;
AudioConvert_I16toF32 intToFloat;
AudioConvert_F32toI16 floatToInt;
AudioInputI2S           adc;
AudioOutputI2S           dac;


AudioConnection          patchCord1(adc, 0, intToFloat, 0);
AudioConnection_F32      patchCord2(intToFloat, 0, fS, 0);
AudioConnection_F32      patchCord3(fS, 0, floatToInt, 0);
AudioConnection          patchCord4(floatToInt, 0, dac, 0);
AudioConnection          patchCord5(floatToInt, 0, dac, 1);
BAPhysicalControls controls(BA_EXPAND_NUM_SW, BA_EXPAND_NUM_POT, 0, BA_EXPAND_NUM_LED);

float fine = 110;
float coarse = 2;

extern "C" int main() {


  TGA_PRO_REVB();
  
  pinMode(BA_EXPAND_LED1_PIN, OUTPUT);

  digitalWrite(BA_EXPAND_LED1_PIN, HIGH);
  delay(100);
  digitalWrite(BA_EXPAND_LED1_PIN, LOW);

  fS.frequency(fine + coarse);
  fS.amplitude(0.5);

  delay(5);
  AudioMemory(12);
  AudioMemory_F32(12);
  codecControl.enable();

  controls.addPot(BA_EXPAND_POT1_PIN, 0, 1020, true);
  controls.addPot(BA_EXPAND_POT2_PIN, 0, 1020, true);
  controls.addPot(BA_EXPAND_POT3_PIN, 0, 1020, true);


  while (1) {

    float value;
    if (controls.checkPotValue(0, value)) {
      fine = static_cast<float>(value)*100.0;
      fS.frequency(fine + coarse);
    }
    
    if (controls.checkPotValue(1, value)) {
      coarse = static_cast<float>(value)*10000.0;
      fS.frequency(fine + coarse);

    }
    
    if (controls.checkPotValue(2, value)) {
  codecControl.setHeadphoneVolume(static_cast<float>(value));
    }

  }
  
}
