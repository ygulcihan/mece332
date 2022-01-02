#include <Arduino.h>
#include <Keypad.h>
#include <Servo.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// Function Declarations
void stateHandler(), lcdWelcome(), checkReset(), sonucCheck(), keypadTest(), passTest(), showBattTemp();
void specialButtonCheck(), charge(), lcdCustomCreate(), elliKurus(), birLira(), servoTest();
bool passBlocked(), battOverheat();
int getNtcTemp();

LiquidCrystal_I2C lcd(0x27, 16, 2);
Servo servo;

enum stateMachine
{
  loading,
  insertCoin,
  charging,
  passCheck,
  passLocked,
  batteryCheck
};
stateMachine state;
stateMachine prevState;

// NTC Definitions
#define ntcPin A0

String sifre = "1120";
String girilenSifre = "";
bool locked = false;
unsigned int kalanDeneme = 2;
unsigned int starPos = 1;

unsigned int ucret = 2;
unsigned int sarjDk = 1;

volatile float sonuc = 0;
volatile bool liraDetected, kurusDetected;

#define role 6
#define ls1 2
#define ls05 3

unsigned long timer = 0;
volatile unsigned int dk, sn, dkUp, snUp;
unsigned long checkDelay, checkDelayPrev;
unsigned long msCounter = 0;
unsigned long msCounterPrev = 0;
unsigned int cancelCounter = 0;
unsigned int lsDisableCounter = 0;
bool interruptsDisabled = true;
bool lcdCleared = true;

// Keypad Definitions
unsigned int wrongPassDelaySec = 30;
unsigned long wrongPassCounter, wrongPassCounterPrev, wrongPassDelayCounter;
const byte ROWS = 4;
const byte COLS = 3;
char keys[ROWS][COLS] =
    {
        {'1', '2', '3'},
        {'4', '5', '6'},
        {'7', '8', '9'},
        {'*', '0', '#'}};
byte rowPins[ROWS] = {12, 11, 10, 9};
byte colPins[COLS] = {8, 7, 6};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);
char key;

// Lcd Custom Characters
byte degreeSign[8] = {0b01100, 0b10010, 0b10010, 0b01100, 0b00000, 0b00000, 0b00000, 0b00000};
byte hourglass[8] = {0b11111, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b11111, 0b00000};
byte checkMark[8] = {0b00000, 0b00001, 0b00011, 0b10110, 0b11100, 0b01000, 0b00000, 0b00000};
byte crossMark[8] = {0b10001, 0b11011, 0b11111, 0b01110, 0b01110, 0b11111, 0b11011, 0b10001};
byte skull[8] = {0b00000, 0b01110, 0b10101, 0b11011, 0b01110, 0b01110, 0b00000, 0b00000};
byte lock[8] = {0b01110, 0b10001, 0b10001, 0b11111, 0b11011, 0b11011, 0b11111, 0b00000};
byte unlock[8] = {0b01110, 0b10000, 0b10000, 0b11111, 0b11011, 0b11011, 0b11111, 0b00000};
byte bar[8] = {0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111};
byte pofu1[8] = {0b10101, 0b11111, 0b10000, 0b10110, 0b10000, 0b10111, 0b10000, 0b11111};
byte pofu2[8] = {0b10101, 0b11111, 0b00001, 0b01101, 0b00001, 0b11101, 0b00001, 0b11111};

void setup()
{
  Serial.begin(9600);
  lcd.begin();
  lcdCustomCreate();
  pinMode(ls1, INPUT);
  pinMode(ls05, INPUT);
  pinMode(role, OUTPUT);
  digitalWrite(role, HIGH);
  sonuc = 0;
  wrongPassDelayCounter = wrongPassDelaySec;
  servo.attach(5);
  state = loading;
  stateHandler();
  state = insertCoin;
}

void loop()
{
  stateHandler();
}

void elliKurus()
{
  if (!kurusDetected)
  {
    if (state == insertCoin)
    {
      sonuc = sonuc + 0.5;
    }

    else if (state == charging)
    {
      dkUp += 7;
      snUp += 30;
      dk += dkUp + (snUp / 60);
      sn += snUp % 60;
      dkUp = 0;
      snUp = 0;
    }
  }
  kurusDetected = true;
}

void birLira()
{
  if (!liraDetected)
  {
    if (state == insertCoin)
    {
      sonuc = sonuc + 1;
    }

    else if (state == charging)
    {
      dkUp += 15;
      dk += dkUp + (snUp / 60);
      sn += snUp % 60;
      dkUp = 0;
      snUp = 0;
    }
  }
  liraDetected = true;
}

void lcdCustomCreate()
{
  lcd.createChar(0, degreeSign);
  lcd.createChar(1, crossMark);
  lcd.createChar(2, pofu1);
  lcd.createChar(3, pofu2);
  lcd.createChar(4, lock);
  lcd.createChar(5, unlock);
  lcd.createChar(6, bar);
  lcd.createChar(7, checkMark);
}

void lcdWelcome()
{
  lcd.setCursor(0, 0);
  lcd.print("Hosgeldiniz! ");
  lcd.setCursor(13, 0);
  lcd.write((byte)2);
  lcd.setCursor(14, 0);
  lcd.write((byte)3);
  lcd.setCursor(15, 0);
  lcd.print(" ");
  lcd.setCursor(0, 1);
  lcd.print("Bakiye = " + String(sonuc) + " tl ");
}

void sonucCheck()
{
  if (sonuc >= ucret)
  {
    timer = sarjDk * 60000;
    dk = timer / 60000;

    state = charging;
  }
}

void checkReset()
{
  checkDelay = millis();

  if (checkDelay - checkDelayPrev >= 1000)
  {
    liraDetected = false;
    kurusDetected = false;
    checkDelayPrev = checkDelay;
  }
}

void servoLock()
{
  if (!locked)
  {
    servo.write(90);
    locked = true;
  }
}

void servoUnlock()
{
  if (locked)
  {
    servo.write(0);
    locked = false;
  }
}

void keypadTest()
{
  key = keypad.getKey();

  if (key != NO_KEY)
  {
    Serial.println(key);
  }
}

void passTest()
{
  lcd.setCursor(0, 0);
  lcd.print("Sifre Giriniz   ");

  key = keypad.getKey();

  if (key != NO_KEY)
  {
    girilenSifre += key;
    lcd.setCursor(starPos, 1);
    lcd.write('*');
    starPos++;
  }

  if (girilenSifre.length() == sifre.length())
  {
    if (girilenSifre == sifre)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sifre Dogru");
      lcd.setCursor(12, 0);
      lcd.write((byte)7);
      lcd.setCursor(0, 1);
      lcd.print("Kilit Aciliyor");
      lcd.setCursor(15, 1);
      lcd.write((byte)5);
      delay(200);
      servoUnlock();
      delay(1500);

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Kasa Durumu:");
      lcd.setCursor(12, 0);
      lcd.write((byte)5);
      lcd.setCursor(11, 1);
      lcd.print("#-->");
      lcd.setCursor(15, 1);
      lcd.write((byte)4);
      while (keypad.getKey() != '#')
      {
      }
      servoLock();
      delay(1500);
      starPos = 1;
      girilenSifre = "";
      state = insertCoin;
    }

    else
    {
      lcd.setCursor(0, 0);
      lcd.print("Sifre Yanlis ");
      lcd.setCursor(13, 0);
      lcd.write((byte)1);
      lcd.setCursor(14, 0);
      lcd.print("  ");
      lcd.setCursor(0, 1);
      lcd.print("  Kalan Deneme:" + String(kalanDeneme));
      delay(2000);
      lcd.setCursor(0, 1);
      lcd.print("               ");

      girilenSifre = "";

      if (kalanDeneme == 0)
      {
        state = passLocked;
      }

      else
      {
        kalanDeneme--;
      }
      starPos = 1;
    }
  }
}

void specialButtonCheck()
{
  key = keypad.getKey();

  if (key == '*')
  {
    state = batteryCheck;
  }

  if (key == '#')
  {
    state = passCheck;
  }

  if (key == '0')
  {
    if (lsDisableCounter < 3)
    {
      lsDisableCounter++;
    }

    else
    {
      if (!interruptsDisabled)
      {
        detachInterrupt(0);
        detachInterrupt(1);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Interrupts:");
        lcd.setCursor(7, 1);
        lcd.print("Disabled");
        interruptsDisabled = true;
      }

      else
      {
        attachInterrupt(digitalPinToInterrupt(ls1), elliKurus, FALLING);
        attachInterrupt(digitalPinToInterrupt(ls05), birLira, FALLING);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Interrupts:");
        lcd.setCursor(7, 1);
        lcd.print("Enabled");
        interruptsDisabled = false;
      }

      delay(1000);
      lsDisableCounter = 0;
    }
  }
}

void showBattTemp()
{
  while (keypad.getKey() != '#')
  {
    int battTemp = getNtcTemp();

    lcd.setCursor(0, 0);
    lcd.print("Bat. Sicakligi: ");
    lcd.setCursor(1, 1);
    lcd.print(String(battTemp));
    lcd.setCursor(4, 1);
    lcd.write((byte)0);
    lcd.setCursor(5, 1);
    lcd.print("C   #->Geri");
  }

  state = insertCoin;
}

void charge()
{
  digitalWrite(role, LOW);

  checkReset();

  if (keypad.getKey() == '#')
  {
    cancelCounter++;
  }
  if (cancelCounter >= 3)
  {
    dk = 0;
    sn = 0;
    dkUp = 0;
    snUp = 0;
    sonuc = 0;
    cancelCounter = 0;
    lcd.clear();
    lcd.setCursor(3, 0);
    lcd.print("Sarj Iptal");
    delay(2000);
    state = insertCoin;
  }

  lcd.setCursor(0, 0);
  lcd.print("Sarj Ediliyor...");

  msCounter = 0;
  msCounter = millis();

  if (sn == 0)
  {
    if (dk != 0)
    {
      dk--;
      sn = 60;
    }

    else
    {
      digitalWrite(role, HIGH);
      lcd.clear();
      sonuc = 0;
      state = insertCoin;
    }
  }

  if (msCounter - msCounterPrev >= 1000)
  {
    sn--;
    msCounterPrev = msCounter;

    if (sn > 60)
    {
      for (unsigned int i = 0; i < (sn / 60); i++)
      {
        dk++;
        sn -= 60;
      }
    }

    lcd.setCursor(0, 1);

    if (sn >= 10 && dk >= 10)
    {
      lcd.print("Kalan: " + String(dk) + ":" + String(sn));
    }

    else if (sn < 10 && dk >= 10)
    {
      lcd.print("Kalan: " + String(dk) + ":0" + String(sn));
    }

    else if (dk < 10 && sn >= 10)
    {
      lcd.print("Kalan: 0" + String(dk) + ":" + String(sn));
    }

    else
    {
      lcd.print("Kalan: 0" + String(dk) + ":0" + String(sn));
    }
  }
}

bool passBlocked()
{
  lcd.setCursor(1, 0);
  lcd.write((byte)1);
  lcd.setCursor(2, 0);
  lcd.print(" Engellendi");
  lcd.setCursor(14, 0);
  lcd.write((byte)1);

  wrongPassCounter = millis();

  if (wrongPassDelayCounter != 0)
  {
    if (wrongPassCounter - wrongPassCounterPrev >= 1000)
    {
      lcd.setCursor(1, 1);
      lcd.print("Kalan: " + String(wrongPassDelayCounter) + " sn");
      wrongPassDelayCounter--;
      wrongPassCounterPrev = wrongPassCounter;
    }
    return true;
  }

  else
  {
    wrongPassDelayCounter = wrongPassDelaySec;
    kalanDeneme = 2;
    state = insertCoin;
    return false;
  }
}

void startup()
{
  lcd.setCursor(0, 0);
  lcd.write((byte)2);
  lcd.setCursor(1, 0);
  lcd.write((byte)3);
  lcd.setCursor(2, 0);
  lcd.print(" Pofuduk Inc.");
  lcd.setCursor(0, 1);
  lcd.print("[");
  lcd.setCursor(15, 1);
  lcd.print("]");
  delay(1500);

  for (int i = 1; i < 15; i++)
  {
    lcd.setCursor(i, 1);
    lcd.write((byte)6);
    delay(50);
  }

  delay(800);
}

void servoTest()
{
  servo.write(0);
  delay(1500);
  servo.write(90);
  delay(1500);
}

int getNtcTemp()
{
  double temp = log(((10240000 / analogRead(ntcPin)) - 10000));
  temp = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * temp * temp)) * temp);
  temp -= 273.15;
  return int(temp);
}

bool battOverheat()
{
  int temp = getNtcTemp();
  if (temp >= 30)
  {
    return true;
  }

  else
  {
    return false;
  }
}

void stateHandler()
{
  if (prevState != state)
  {
    lcd.clear();
    prevState = state;
  }

  switch (state)
  {
  case loading:
    startup();
    break;
  case insertCoin:
    lcdWelcome();
    checkReset();
    sonucCheck();
    specialButtonCheck();
    break;
  case charging:
    if (battOverheat() == true)
    {
      digitalWrite(role, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Bat. Sicakligi");
      lcd.setCursor(2, 1);
      lcd.print("Fazla Yuksek");
      delay(3000);
      state = batteryCheck;
    }
    charge();
    break;
  case passCheck:
    passTest();
    break;
  case passLocked:
    passBlocked();
    break;
  case batteryCheck:
    showBattTemp();
    break;
  }
}