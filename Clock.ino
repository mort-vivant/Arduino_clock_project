#include <DS3232RTC.h>
#include <Time.h>
#include <TimeLib.h>
#include <Wire.h>
#include "FastLED.h"
#include "DHT.h"
#define DHT_PIN 5             // пин подключения датчика DHT 11 
#define DHTTYPE DHT11         // DHT 11 
#define NUM_LEDS 114          // 4*7*4 +2  количество светодиодов в ленте
#define COLOR_ORDER BRG       // определение порядка цветов для ленты
#define DATA_PIN 6            // пин подключения ленты
#define DST_PIN 2             // пин кнопки перевода на летнее время и обратно
#define MIN_PIN 3             // пин кнопки настройки минут
#define HUR_PIN 4             // пин кнопки настройки часов
#define BRI_PIN A3            // фоторезистор
#define auto_bright 1         // автоматическая подстройка яркости от уровня внешнего освещения (1 - включить, 0 - выключить)
#define max_bright 50         // максимальная яркость (0 - 255)
#define min_bright 1          // минимальная яркость (0 - 255)
#define bright_constant 500   // константа усиления от внешнего света (0 - 1023), чем МЕНЬШЕ константа, тем "резче" будет прибавляться яркость
#define coef 0.9              // коэффициент фильтра (0.0 - 1.0), чем больше - тем медленнее меняется яркость
int new_bright, new_bright_f;
unsigned long bright_timer, off_timer;

DHT dht(DHT_PIN, DHTTYPE);

CRGB leds[NUM_LEDS]; // Define LEDs strip
                     // 0,0,0,0
                     // 1,1,1,1
                     // 1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28
byte digits[13][28] = {{0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 0
                       {0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},   // Digit 1
                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},   // Digit 2
                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 3
                       {1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},   // Digit 4
                       {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 5
                       {1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 6
                       {0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1},   // Digit 7
                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 8
                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1},   // Digit 9 | 2D Array for numbers on 7 segment,
                       {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},   // ° char
                       {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0},   // C char
                       {1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1}};  // % char

bool Dot = true;  //Dot state
bool DST = false; //DST state
bool TempShow = false;
bool HumidityShow = false;
bool led_state = true;  // флаг состояния ленты
int last_digit = 0;
// int ledColor = 0x0000FF; // Color used (in hex)
long ledColor = CRGB::DarkOrchid; // Color used (in hex)
//long ledColor = CRGB::MediumVioletRed;
//Random colors i picked up
long ColorTable[16] = {
  CRGB::Amethyst,
  CRGB::Aqua,
  CRGB::Blue,
  CRGB::LawnGreen,
  CRGB::MediumSeaGreen,
  CRGB::DarkMagenta,
  CRGB::Coral,
  CRGB::DeepPink,
  CRGB::Fuchsia,
  CRGB::Gold,
  CRGB::Indigo,
  CRGB::LightCoral,
  CRGB::Tomato,
  CRGB::Salmon,
  CRGB::OrangeRed,
  CRGB::Orchid
};
void setup() {
  Serial.begin(9600);
  LEDS.addLeds<WS2812B, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS); // Set LED strip type
  LEDS.setBrightness(30); // Set initial brightness
  pinMode(DST_PIN, INPUT_PULLUP); // Define DST adjust button pin
  pinMode(MIN_PIN, INPUT_PULLUP); // Define Minutes adjust button pin
  pinMode(HUR_PIN, INPUT_PULLUP); // Define Hours adjust button pin
  TempShow = false;
  HumidityShow = false;
  dht.begin();
}

// Get time in a single number, if hours will be a single digit then time will be displayed 155 instead of 0155
int GetTime() {
  tmElements_t Now;
  RTC.read(Now);
  //time_t Now = RTC.Now();// Getting the current Time and storing it into a DateTime object
  int hour = Now.Hour;
  int minutes = Now.Minute;
  int second = Now.Second;
  if (second % 2 == 0) {
    Dot = false;
  }
  else {
    Dot = true;
  };
  return (hour * 100 + minutes);
};

// Check Light sensor and set brightness accordingly
void BrightnessCheck() {

  if (auto_bright) {                         // если включена адаптивная яркость
    if (millis() - bright_timer > 100) {     // каждые 100 мс
      bright_timer = millis();               // сброить таймер
      new_bright = map(analogRead(BRI_PIN), 0, bright_constant, min_bright, max_bright);   // считать показания с фоторезистора, перевести диапазон
      new_bright = constrain(new_bright, min_bright, max_bright);
      new_bright_f = new_bright_f * coef + new_bright * (1 - coef);
      LEDS.setBrightness(new_bright_f);      // установить новую яркость
    }
  }
   if (!led_state) led_state = true;
  off_timer = millis();  
};

// Convert time to array needet for display
void TimeToArray() {
  int Now = GetTime();  //  Получить текущее время

  int cursor = 114; // номер последнего светодиода

  // Serial.print("Time is: ");Serial.println(Now);
  if (DST) {  // if DST is true then add one hour
    Now += 100;
    // Serial.print("DST is ON, time set to : ");Serial.println(Now);
  };
  if (Dot) {
    leds[57] = ledColor;
    leds[56] = ledColor;
  }
  else  {
    leds[57] = 0x000000;
    leds[56] = 0x000000;
  };

  for (int i = 1; i <= 4; i++) {
    int digit = Now % 10; // get last digit in time
    if (i == 1) {
      //  Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 86;

      for (int k = 0; k <= 27; k++) {
        // Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
      if (digit != last_digit)
      {
        cylon();
        ledColor =  ColorTable[random(16)];
      }
      last_digit = digit;

    }
    else if (i == 2) {
      // Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 58;

      for (int k = 0; k <= 27; k++) {
        // Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
    }
    else if (i == 3) {
      // Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 28;
      for (int k = 0; k <= 27; k++) {
        // Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
    }
    else if (i == 4) {
      // Serial.print("Digit 1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 27; k++) {
        // Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
    }
    Now /= 10;
  };
};
// Convert temp to array needet for display
void TempToArray() {
  tmElements_t tm;
  RTC.read(tm);
  if (tm.Second != 17) {
    TempShow = false;
    return;
  }
  TempShow = true;
  int t = dht.readTemperature();
  int celsius = t * 100;
  //   Serial.print("Temp is: ");Serial.println(celsius);

  int cursor = 114; // last led number

  leds[57] = 0x000000;
  leds[56] = 0x000000;

  for (int i = 1; i <= 4; i++) {
    int digit = celsius % 10; // get last digit in time
    if (i == 1) {
      // Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 86;

      for (int k = 0; k <= 27; k++) {
        //  Serial.print(digits[11][k]);
        if (digits[11][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[11][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    else if (i == 2) {
      //  Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 58;

      for (int k = 0; k <= 27; k++) {
        //   Serial.print(digits[10][k]);
        if (digits[10][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[10][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    else if (i == 3) {
      //  Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 28;
      for (int k = 0; k <= 27; k++) {
        //  Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
    }
    else if (i == 4) {
      //  Serial.print("Digit 1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 27; k++) {
        //   Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    celsius /= 10;
  };
};

void HumidityToArray() {
  tmElements_t tm;
  RTC.read(tm);
  if (tm.Second != 38) {
    HumidityShow = false;
    return;
  }
  HumidityShow = true;
  int h = dht.readHumidity();
  int humidity_percentage = h * 100;
  //   Serial.print("Humidity is: ");Serial.println(humidity_percentage);

  int cursor = 114; // last led number

  leds[57] = 0x000000;
  leds[56] = 0x000000;

  for (int i = 1; i <= 4; i++) {
    int digit = humidity_percentage % 10; // get last digit in time
    if (i == 1) {
      // Serial.print("Digit 4 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 86;

      for (int k = 0; k <= 27; k++) {
        //  Serial.print(digits[11][k]);
        if (digits[12][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[12][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    else if (i == 2) {
      //  Serial.print("Digit 3 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 58;

      for (int k = 0; k <= 27; k++) {
        //   Serial.print(digits[10][k]);
        if (digits[10][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[10][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    else if (i == 3) {
      //  Serial.print("Digit 2 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 28;
      for (int k = 0; k <= 27; k++) {
        //  Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      // Serial.println();
    }
    else if (i == 4) {
      //  Serial.print("Digit 1 is : ");Serial.print(digit);Serial.print(" ");
      cursor = 0;
      for (int k = 0; k <= 27; k++) {
        //   Serial.print(digits[digit][k]);
        if (digits[digit][k] == 1) {
          leds[cursor] = ledColor;
        }
        else if (digits[digit][k] == 0) {
          leds[cursor] = 0x000000;
        };
        cursor ++;
      };
      //  Serial.println();
    }
    humidity_percentage /= 10;
  };
};

void DSTcheck() {
  int buttonDST = digitalRead(2);
  // Serial.print("DST is: ");Serial.println(DST);
  if (buttonDST == LOW) {
    if (DST) {
      DST = false;
      // Serial.print("Switching DST to: ");Serial.println(DST);
    }
    else if (!DST) {
      DST = true;
      // Serial.print("Switching DST to: ");Serial.println(DST);
    };
    delay(500);
  };
}
void TimeAdjust() {
  int buttonH = digitalRead(HUR_PIN);
  int buttonM = digitalRead(MIN_PIN);
  if (buttonH == LOW || buttonM == LOW) {
    delay(500);
    tmElements_t Now;
    RTC.read(Now);
    int hour = Now.Hour;
    int minutes = Now.Minute;
    int second = Now.Second;
    if (buttonH == LOW) {
      if (Now.Hour == 23) {
        Now.Hour = 0;
      }
      else {
        Now.Hour += 1;
      };
    } else {
      if (Now.Minute == 59) {
        Now.Minute = 0;
      }
      else {
        Now.Minute += 1;
      };
    };
    RTC.write(Now);
  }
}
/* coool effect function*/
void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(250);
  }
}
void cylon () {
  static uint8_t hue = 0;
  // Serial.print("x");
  // First slide the led in one direction
  for (int i = 0; i < NUM_LEDS; i++) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
  // Serial.print("x");
  // Now go in the other direction.
  for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
    // Set the i'th led to red
    leds[i] = CHSV(hue++, 255, 255);
    // Show the leds
    FastLED.show();
    // now that we've shown the leds, reset the i'th led to black
    // leds[i] = CRGB::Black;
    fadeall();
    // Wait a little bit before we loop around and do it again
    delay(10);
  }
}

void loop()  // Main loop
{
  BrightnessCheck(); // Проверка яркости
  DSTcheck(); // Check DST
  TimeAdjust(); // Check to se if time is geting modified
  TimeToArray(); // Get leds array with required configuration
  TempToArray();
  HumidityToArray();
  FastLED.show(); // Display leds array
  if (TempShow == true) delay (5000);
  if (HumidityShow == true) delay (5000);
}
