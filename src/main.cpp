#include <M5Unified.h>
#include <Ticker.h>

// PortB: 36/26/V/G
#define PIN_PWM 26 // PortB
#define PIN_ADC 36 // PortB / 1/5 of Voltage at "S" of nMOS (Vsol=12V)

Ticker tickerSet;
volatile int duty = 0;

// every 100ms
void IRAM_ATTR onTimer()
{
  //   ledcWrite(0, duty);
}

#define DUTY_MIN 10
// @10Hz: T=100ms, Toff>15ms -> duty<85% (204/255)
// #define DUTY_MAX 200
// @5Hz: T=200ms, Toff>15ms -> duty<92% (235/255)
#define DUTY_MAX 235

void setup()
{
  M5.begin();
  M5.Lcd.clear();
  pinMode(PIN_PWM, OUTPUT);
  ledcSetup(0, 1000, 10); // 1000Hz, 10bit
  //ledcSetup(0, 1000, 8); // 1000Hz, 8bit
  //ledcSetup(0, 10, 8); // 10Hz
  //ledcSetup(0, 100, 8); // 100Hz
  //ledcSetup(0, 50, 8); // 50Hz
  ledcAttachPin(PIN_PWM, 0);
  ledcWrite(0, 20); // initial duty
  //      tickerSet.attach_ms(100, onTimer);
  pinMode(33, OUTPUT);
  digitalWrite(33, 0);
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

// for 12V, divider=1:4
//#define VTH1 1500 // [mV], rising edge of PWM
//#define VTH2 2700 // [mV], dropping edge of PWM
// for 18V, divider=1:10
//#define VTH1 1000 // [mV], rising edge of PWM
//#define VTH2 1820 // [mV], dropping edge of PWM
// for 24V, divider=1:10
#define VTH1 1200 // [mV], rising edge of PWM
#define VTH2 2400 // [mV], dropping edge of PWM
uint8_t st = 0;
uint16_t tm1 = 0, tm2 = 0;

uint8_t pos_t = 50;

void loop()
{
  if (M5.BtnA.wasPressed())
  {
    pos_t -= 10;
    if (pos_t < 0)
      pos_t = 0;
    printf("pos_t=%d\n", pos_t);
  }

  duty = 80; // [%]

  int pos;
  // PWM drive
  ledcWrite(0, (int)((float)duty / 100.0 * 1024)); // set duty
  delay(100);

  // position measure
  // Solenoid ON for 10ms
  ledcWrite(0, 1023); // ON
  digitalWrite(33, 1); // PortA-1
#define T_MEAS_ON 20
  delay(T_MEAS_ON);
  // Sonlenoid OFF
  ledcWrite(0, 0); // OFF
  digitalWrite(33, 0); // PortA-1
  st = 0;
  while (st < 2)
  {
    int v = analogReadMilliVolts(PIN_ADC);
    if (st == 0 && v > VTH1)
    {
      st = 1;
      //          tm1 = millis(); // [ms]
      tm1 = esp_timer_get_time(); // [us]
    }
    else if (st == 1 && v < VTH2)
    {
      st = 2;
      //          tm2 = millis(); [ms]
      tm2 = esp_timer_get_time(); // [us]
      // tm2-tm1 : transient time
      // 2500us(neutral)-4500us(pulled) @ 100Hz
      // 4500us(neutral)-13500us(pulls) @ 100Hz
      // 2950us(neutral)-5250us(pulls) @ manual measure @ 12V
      // 4050us(neutral)-8250us(pulls) @ manual measure @ 18V
      // 2370us(neutral)-4070us(pulls) @ manual measure @ 24V
      if (tm2 > tm1)
      {
        //         printf(">t:%d\n", tm2 - tm1); // for Teleplot
        pos = (tm2 - tm1 - 2370) / 17;
        if (pos < 0)
          pos = 0;
        else if (pos > 100)
          pos = 100;
        //          duty = (pos_t - pos) * 10; // PID control
        // tentative position control (doesn't reflect to duty now)
        if (pos > pos_t)
          duty--;
        else if (pos < pos_t)
          duty++;
        if (duty < DUTY_MIN)
          duty = DUTY_MIN;
        else if (duty > DUTY_MAX)
          duty = DUTY_MAX;
      }

      uint16_t toff_tran = tm2 - tm1; // transient time according to position

      // keep duty constant as that during non-measurement period
      // Tmeasure = T_MEAS_ON + toff_tran + t_on_tran
      // duty = (T_MEAS_ON + t_on_tran) / Tmeasure * 100 [%]
      // T_MEAS_ON = 1ms, t_off_tran = 4-11ms
      // t_on_tran = ((100-d)* T_MEAS_ON - duty * toff_tran) / (duty - 100) [ms]
      uint16_t t_on_tran = ((100-duty)* T_MEAS_ON - duty * toff_tran) / (duty - 100);
      float duty0 = (float)(T_MEAS_ON + t_on_tran) / (T_MEAS_ON + toff_tran + t_on_tran) * 100;
      printf("%d %d %.2f %d %d %d\n", toff_tran, t_on_tran, duty0, pos, pos_t, duty);
//      ledcWrite(0, 1023);
//      delay(t_on_tran / 1000);
    }
  }
}
