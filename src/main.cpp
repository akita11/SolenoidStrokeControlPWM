#include <M5Unified.h>

#define PIN_PWM 22 // PortA-B (22/21/V/G)
#define PIN_ADC 36 // PortB-B (36/26/V/G) // Vsol=12V / divider=4


void setup() {
  M5.begin();
  M5.Display.setTextSize(2);;
}

#define VTH1 1500 // [mV]
#define VTH2 3200 // [mV]
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
  }
  else if (st == 2 && v < VTH1){
     st = 0;
  }

  M5.Display.clear();

  M5.Display.setCursor(0, 0);
  M5.Display.printf("%d", v);
  M5.Display.printf("%d", tm2 - tm1);
}
