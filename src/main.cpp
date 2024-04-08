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
    ledcSetup(0, 1000, 8); // 1000Hz, 8bit
    //    ledcSetup(0, 10, 8); // 10Hz
    //    ledcSetup(0, 100, 8); // 100Hz
    //    ledcSetup(0, 50, 8); // 50Hz
    ledcAttachPin(PIN_PWM, 0);
    ledcWrite(0, 20); // initial duty
    //      tickerSet.attach_ms(100, onTimer);
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

uint8_t pos_t = 50;

void loop()
{
    int pos;
    // PWM drive
    ledcAttachPin(PIN_PWM, 0);
    ledcWrite(0, 20); // initial duty
    delay(100);

    // position measure
    ledcDetachPin(PIN_PWM);
    pinMode(PIN_PWM, OUTPUT);
    // Solenoid ON for 10ms
    digitalWrite(PIN_PWM, 1);
    delay(10);
    // Sonlenoid OFF
    digitalWrite(PIN_PWM, 0);
    while(st < 2){
        int v = analogReadMilliVolts(PIN_ADC);
/*
      if (M5.BtnA.wasPressed())
      {
            pos_t -= 10;
            if (pos_t < 0)
                  pos_t = 0;
            printf("pos_t=%d\n", pos_t);
      }
*/
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
            if (tm2 > tm1)
            {
                //         printf(">t:%d\n", tm2 - tm1); // for Teleplot
                pos = (tm2 - tm1 - 4500) / 90;
                if (pos < 0)
                    pos = 0;
                    else if (pos > 100)
                        pos = 100;
                    //          duty = (pos_t - pos) * 10; // PID control
                    if (pos > pos_t)
                        duty--;
                    else if (pos < pos_t)
                        duty++;
                    if (duty < DUTY_MIN)
                        duty = DUTY_MIN;
                    else if (duty > DUTY_MAX)
                        duty = DUTY_MAX;
            	}
            }
            else if (st == 2 && v < VTH1)
            {
                st = 3;
            }
    }
    printf("%d %d %d %d\n", tm2 - tm1, pos, pos_t, duty);
}
