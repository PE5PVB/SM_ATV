#include <SPI.h>
#include <RotaryEncoder.h>          // https://github.com/enjoyneering/RotaryEncoder
#include <Adafruit_SSD1306.h>       // https://github.com/adafruit/Adafruit_SSD1306
#include <EEPROMex.h>               // https://github.com/thijse/Arduino-EEPROMEx

#define BAND            13          // 23 = 23cm, 13 = 13cm, 6 = 6cm

#define LED             8
#define LD              9
#define VIDEO           6
#define PA              15
#define ROTARY_PIN_A    2
#define ROTARY_PIN_B    3
#define ROTARY_BUTTON   4
#define STANDBY_BUTTON  5

#if BAND == 23
#define LOWEDGEMIN      1240
#define HIGHEDGEMAX     1300
#define DEFAULTCENTER   1250000
#define BOOTTEXT        "23 CM ATV"
#endif
#if BAND == 13
#define LOWEDGEMIN      2320
#define HIGHEDGEMAX     2400
#define DEFAULTCENTER   2350000
#define BOOTTEXT        "13 CM ATV"
#endif
#if BAND == 6
#define LOWEDGEMIN      5700
#define HIGHEDGEMAX     5800
#define DEFAULTCENTER   5750000
#define BOOTTEXT        " 6 CM ATV"
#endif

const char* const MENU[]
{
  "<CONTRAST>",
  "<STEPSIZE>",
  "<LOW EDGE>",
  "<HIGHEDGE>",
  "<  EXIT  >"
};

const char* const MENU_CONTRAST[]
{
  "   HIGH   ",
  "   LOW    "
};

const char* const MENU_STEPSIZE[]
{
  " 1000 kHz ",
  "  500 kHz ",
  "  100 kHz "
};

bool direction;
bool flasher;
bool lockset = false;
bool menu = false;
bool menuset = false;
bool tune = false;
byte contrastset;
byte menuoption;
bool standbyset;
byte stepsizeset;
int16_t position;
unsigned int highedge;
unsigned int lowedge;
unsigned int stepsize;
unsigned long frequency;
unsigned long frequencyold;
unsigned long timer;

Adafruit_SSD1306 oled(128, 32, &Wire, -1);
RotaryEncoder RotaryEncoder(ROTARY_PIN_A, ROTARY_PIN_B, -1);

void encoderISR()
{
  RotaryEncoder.readAB();
}

void FactoryDefaults() {
  EEPROM.write(0, 1);
  EEPROM.write(1, 0);
  EEPROM.write(2, 0);
  EEPROM.write(3, 0);
  EEPROM.writeInt(4, LOWEDGEMIN);
  EEPROM.writeInt(6, HIGHEDGEMAX);
  EEPROM.writeLong(8, DEFAULTCENTER);
}

void setup() {
  byte checkdata;
  checkdata = EEPROM.read(0);
  if (checkdata != 1) {
    FactoryDefaults();
  }

  contrastset = EEPROM.read(1);
  stepsizeset = EEPROM.read(2);
  standbyset = EEPROM.read(3);
  lowedge = EEPROM.readInt(4);
  highedge = EEPROM.readInt(6);
  frequency = EEPROM.readLong(8);

  pinMode(LED, OUTPUT);
  pinMode(VIDEO, OUTPUT);
  pinMode(PA, OUTPUT);
  pinMode(ROTARY_BUTTON, INPUT_PULLUP);
  pinMode(STANDBY_BUTTON, INPUT_PULLUP);
  digitalWrite(PA, HIGH);

  RotaryEncoder.begin();
  SPI.begin();
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_A),  encoderISR,       CHANGE);
  attachInterrupt(digitalPinToInterrupt(ROTARY_PIN_B),  encoderISR,       CHANGE);

  oled.clearDisplay();
  oled.drawRoundRect(0, 0, 127, 32, 8, SSD1306_WHITE);
  oled.setTextSize(2);
  oled.setTextColor(SSD1306_WHITE);
  if (BAND != 6) {
    oled.setCursor(8, 9);
  } else {
    oled.setCursor(5, 9);
  }
  oled.print(BOOTTEXT);
  oled.display();
  delay(1500);
  oled.setTextColor(SSD1306_BLACK);
  if (BAND != 6) {
    oled.setCursor(8, 9);
  } else {
    oled.setCursor(5, 9);
  }
  oled.print(BOOTTEXT);

  if (stepsizeset == 0) {
    stepsize = 1000;
  } else if (stepsizeset == 1) {
    stepsize = 500;
  } else if (stepsizeset == 2) {
    stepsize = 100;
  }
  oled.dim(contrastset);

  if (standbyset == true) {
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(5, 9);
    oled.print("  TX OFF  ");
    oled.display();
  } else {
    SetFreq(frequency, stepsize);
    ShowFreq();
  }

  position = RotaryEncoder.getPosition();
}

void loop() {
  if (standbyset == false) {
    LockDetect();
  }

  if (tune == true) {
    ShowFreqFlasher();
  }

  if (position < RotaryEncoder.getPosition()) {
    KeyUp();
  }

  if (position > RotaryEncoder.getPosition()) {
    KeyDown();
  }

  if (digitalRead(ROTARY_BUTTON) == LOW) {
    ButtonPress();
  }

  if (digitalRead(STANDBY_BUTTON) == LOW) {
    StandbyPress();
  }
}

void StandbyPress() {
  if (menu == false) {
    if (standbyset == false) {
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(5, 9);
      oled.print(frequencyold / 1000);
      oled.print(".");
      oled.print(frequencyold % 1000 / 100);
      oled.println(" MHz");
      oled.setTextColor(SSD1306_WHITE);
      oled.setCursor(5, 9);
      oled.print("  TX OFF  ");
      oled.display();
      tune = false;
      digitalWrite(LED, LOW);
      digitalWrite(PA, HIGH);
      digitalWrite(VIDEO, LOW);
      SetFreq(0, 0);
      standbyset = true;
    } else {
      oled.setTextColor(SSD1306_BLACK);
      oled.setCursor(5, 9);
      oled.print("  TX OFF  ");
      SetFreq(frequency, stepsize);
      ShowFreq();
      standbyset = false;
    }
    EEPROM.write(3, standbyset);
    while (digitalRead(STANDBY_BUTTON) == LOW) {
      delay(100);
    }
    position = RotaryEncoder.getPosition();
  }
}

void ButtonPress() {
  if (standbyset == false) {
    if (tune == true) {
      SetFreq(frequency, stepsize);
      EEPROM.writeLong(8, frequency);
      ShowFreq();
      tune = false;
    } else {
      if (menu == false) {
        menu = true;
        oled.setTextColor(SSD1306_BLACK);
        oled.setCursor(5, 9);
        oled.print(frequencyold / 1000);
        oled.print(".");
        oled.print(frequencyold % 1000 / 100);
        oled.println(" MHz");
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(5, 9);
        oled.println(" - MENU - ");
        oled.display();
        delay(1000);
        oled.setTextColor(SSD1306_BLACK);
        oled.setCursor(5, 9);
        oled.println(" - MENU - ");
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(5, 9);
        oled.println(MENU[menuoption]);
        oled.display();
      } else {
        if (menuset == true) {
          switch (menuoption) {
            case 0:
              menuset = false;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU_CONTRAST[contrastset]);
              break;

            case 1:
              menuset = false;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU_STEPSIZE[stepsizeset]);
              break;

            case 2:
              menuset = false;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.print(" ");
              oled.print(lowedge);
              oled.println(" MHz");
              break;

            case 3:
              menuset = false;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.print(" ");
              oled.print(highedge);
              oled.println(" MHz");
              break;
          }
          oled.setTextColor(SSD1306_WHITE);
          oled.setCursor(5, 9);
          oled.println(MENU[menuoption]);
          oled.display();
        } else {
          switch (menuoption) {
            case 0:
              menuset = true;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU[menuoption]);
              oled.setTextColor(SSD1306_WHITE);
              oled.setCursor(5, 9);
              oled.println(MENU_CONTRAST[contrastset]);
              oled.display();
              break;

            case 1:
              menuset = true;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU[menuoption]);
              oled.setTextColor(SSD1306_WHITE);
              oled.setCursor(5, 9);
              oled.println(MENU_STEPSIZE[stepsizeset]);
              oled.display();
              break;

            case 2:
              menuset = true;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU[menuoption]);
              oled.setTextColor(SSD1306_WHITE);
              oled.setCursor(5, 9);
              oled.print(" ");
              oled.print(lowedge);
              oled.println(" MHz");
              oled.display();
              break;

            case 3:
              menuset = true;
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU[menuoption]);
              oled.setTextColor(SSD1306_WHITE);
              oled.setCursor(5, 9);
              oled.print(" ");
              oled.print(highedge);
              oled.println(" MHz");
              oled.display();
              break;

            case 4:
              oled.setTextColor(SSD1306_BLACK);
              oled.setCursor(5, 9);
              oled.println(MENU[menuoption]);
              if (frequency / 1000 < lowedge || frequency / 1000 > highedge - 1) {
                frequency = lowedge;
                frequency *= 1000;
                EEPROM.writeLong(8, frequency);
              }
              ShowFreq();
              menuoption = 0;
              menu = false;
              EEPROM.write(1, contrastset);
              EEPROM.write(2, stepsizeset);
              EEPROM.writeInt(4, lowedge);
              EEPROM.writeInt(6, highedge);
              break;
          }
        }
      }
    }
    while (digitalRead(ROTARY_BUTTON) == LOW) {
      delay(100);
    }
  }
}

void KeyUp() {
  if (standbyset == false) {
    if (menu == false) {
      tune = true;
      if (stepsizeset == 0) {
        stepsize = 1000;
      } else if (stepsizeset == 1) {
        stepsize = 500;
      } else if (stepsizeset == 2) {
        stepsize = 100;
      }
      frequency += stepsize;
      if (frequency / 1000 > highedge - 1) {
        frequency = lowedge;
        frequency *= 1000;
      }
      ShowFreq();
    } else {
      if (menuset == false) {
        oled.setTextColor(SSD1306_BLACK);
        oled.setCursor(5, 9);
        oled.println(MENU[menuoption]);
        menuoption ++;
        if (menuoption > 4) {
          menuoption = 0;
        }
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(5, 9);
        oled.println(MENU[menuoption]);
        oled.display();
      } else {
        switch (menuoption) {
          case 0:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.println(MENU_CONTRAST[contrastset]);
            contrastset ++;
            if (contrastset > 1) {
              contrastset = 0;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.println(MENU_CONTRAST[contrastset]);
            oled.dim(contrastset);
            oled.display();
            break;

          case 1:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.println(MENU_STEPSIZE[stepsizeset]);
            stepsizeset ++;
            if (stepsizeset > 2) {
              stepsizeset = 0;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.println(MENU_STEPSIZE[stepsizeset]);
            oled.display();
            break;

          case 2:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(lowedge);
            oled.println(" MHz");
            lowedge ++;
            if (lowedge > highedge - 5) {
              lowedge = LOWEDGEMIN;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(lowedge);
            oled.println(" MHz");
            oled.display();
            break;

          case 3:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(highedge);
            oled.println(" MHz");
            highedge ++;
            if (highedge > HIGHEDGEMAX) {
              highedge = lowedge + 5;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(highedge);
            oled.println(" MHz");
            oled.display();
            break;
        }
      }
    }
    while (digitalRead(ROTARY_PIN_A) == LOW || digitalRead(ROTARY_PIN_B) == LOW) {
    }
    position = RotaryEncoder.getPosition();
  }
}

void KeyDown() {
  if (standbyset == false) {
    if (menu == false) {
      tune = true;
      if (stepsizeset == 0) {
        stepsize = 1000;
      } else if (stepsizeset == 1) {
        stepsize = 500;
      } else if (stepsizeset == 2) {
        stepsize = 100;
      }
      frequency -= stepsize;
      if (frequency / 1000 < lowedge) {
        frequency = highedge - 1;
        frequency *= 1000;
      }
      ShowFreq();
    } else {
      if (menuset == false) {
        oled.setTextColor(SSD1306_BLACK);
        oled.setCursor(5, 9);
        oled.println(MENU[menuoption]);
        menuoption --;
        if (menuoption > 4) {
          menuoption = 4;
        }
        oled.setTextColor(SSD1306_WHITE);
        oled.setCursor(5, 9);
        oled.println(MENU[menuoption]);
        oled.display();
      } else {
        switch (menuoption) {
          case 0:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.println(MENU_CONTRAST[contrastset]);
            contrastset --;
            if (contrastset > 1) {
              contrastset = 1;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.println(MENU_CONTRAST[contrastset]);
            oled.dim(contrastset);
            oled.display();
            break;

          case 1:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.println(MENU_STEPSIZE[stepsizeset]);
            stepsizeset --;
            if (stepsizeset > 2) {
              stepsizeset = 2;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.println(MENU_STEPSIZE[stepsizeset]);
            oled.display();
            break;

          case 2:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(lowedge);
            oled.println(" MHz");
            lowedge --;
            if (lowedge < LOWEDGEMIN) {
              lowedge = highedge - 5;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(lowedge);
            oled.println(" MHz");
            oled.display();
            break;

          case 3:
            oled.setTextColor(SSD1306_BLACK);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(highedge);
            oled.println(" MHz");
            highedge --;
            if (highedge < lowedge + 5) {
              highedge = HIGHEDGEMAX;
            }
            oled.setTextColor(SSD1306_WHITE);
            oled.setCursor(5, 9);
            oled.print(" ");
            oled.print(highedge);
            oled.println(" MHz");
            oled.display();
            break;
        }
      }
    }
    while (digitalRead(ROTARY_PIN_A) == LOW || digitalRead(ROTARY_PIN_B) == LOW) {
    }
    position = RotaryEncoder.getPosition();
  }
}

void LockDetect() {
  if (lockset == false) {
    if (digitalRead(LD) == HIGH) {
      digitalWrite(LED, HIGH);
      digitalWrite(PA, LOW);
      digitalWrite(VIDEO, HIGH);
      lockset = true;
    } else {
      digitalWrite(LED, LOW);
      digitalWrite(PA, HIGH);
      digitalWrite(VIDEO, LOW);
    }
  }
}

void ShowFreqFlasher() {
  if (millis() >= timer + 200) {
    timer = millis();
    if (flasher == true) {
      oled.setTextColor(SSD1306_BLACK);
      flasher = false;
    } else {
      oled.setTextColor(SSD1306_WHITE);
      flasher = true;
    }
    oled.setCursor(5, 9);
    oled.print(frequency / 1000);
    oled.print(".");
    oled.print(frequency % 1000 / 100);
    oled.println(" MHz");
    oled.display();
  }
}

void ShowFreq() {
  oled.setTextColor(SSD1306_BLACK);
  oled.setCursor(5, 9);
  oled.print(frequencyold / 1000);
  oled.print(".");
  oled.print(frequencyold % 1000 / 100);
  oled.println(" MHz");
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(5, 9);
  oled.print(frequency / 1000);
  oled.print(".");
  oled.print(frequency % 1000 / 100);
  oled.println(" MHz");
  oled.display();
  frequencyold = frequency;
}

void SetFreq(unsigned long freq, unsigned int pfd) {
  unsigned int ref = 25000;
  byte prescaler;
  unsigned long b;
  unsigned int a;
  unsigned long r_counter_latch = 0;
  unsigned long n_counter_latch = 1;
  unsigned long function_latch = 0x1F8092;    // CP 5mA, Digital lock detect enabled

  r_counter_latch = ref / pfd * 4;

  if (freq <= 2400000) {
    prescaler = 8;
  } else {
    prescaler = 32;
    function_latch += 0x800000;
  }

  b = (freq / pfd) / prescaler;
  a = (freq / pfd) - (b * prescaler);

  n_counter_latch += b << 8;
  n_counter_latch += a << 2;

  digitalWrite(LED, LOW);
  digitalWrite(PA, HIGH);
  digitalWrite(VIDEO, LOW);
  lockset = false;

  WriteRegister(function_latch);
  WriteRegister(r_counter_latch);
  WriteRegister(n_counter_latch);
}

void WriteRegister(uint32_t data)
{
  digitalWrite(SS, LOW);
  for (int i = 0; i < 4 ; i++)
  {
    uint8_t dataByte = data >> (8 * (3 - i));
    SPI.transfer(dataByte);
  }
  digitalWrite(SS, HIGH);
}
