#include <M5Unified.h>

// PortB: 36/26/V/G
#define PIN_PWM 26 // PortB
#define PIN_ADC 36 // PortB / Vsol=12V / divider=4

void setup() {
  M5.begin();
  ledcSetup(0, 10, 8);
  ledcAttachPin(PIN_PWM, 0);
  ledcWrite(0, 20);
}

/*
      +-----
VTH2  |     | <----
      |     -------+
VTH1  |            |
      |            |
0 ----+            +-----
      tm1   tm2
*/

#define VTH1 1500 // [mV], rising edge of PWM
#define VTH2 2700 // [mV], dropping edge of PWM
uint8_t st = 0;
uint16_t tm1 = 0, tm2 = 0;

void loop() {
  int t;
  int v = analogReadMilliVolts(PIN_ADC);
  if (st == 0 && v > VTH1){
     st = 1;
     tm1 = millis();
  }
  else if (st == 1 && v < VTH2){
      st = 2;
      tm2 = millis();
      printf(">t:%d\n", tm2 - tm1);
  }
  else if (st == 2 && v < VTH1){
     st = 0;
  }
}
