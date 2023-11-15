#include <Arduino.h>
#include <Keypad.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EasyBuzzer.h>
#include <math.h>
#include <stdio.h>
#define ROW_NUM 4
#define COLUMN_NUM 4
#define SENSOR_NUM 2
#define ACTUATOR_NUM 2
#define SCL 22
#define SDA 21

// Keyboard
char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte pin_rows[ROW_NUM] = {16, 4, 0, 2};
byte pin_column[COLUMN_NUM] = {15, 8, 7, 6};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);

// Motors
int motor1[ACTUATOR_NUM] = {25, 26};
int motor2[ACTUATOR_NUM] = {27, 14};

// Buzzer
const int buzzer = 33;
unsigned int beep_success[6] = {1000, 50, 100, 2, 500, 10};
unsigned int beep_failure[6] = {500, 50, 100, 2, 500, 10};
unsigned int frequency = 1000;
unsigned int onDuration = 50;
unsigned int offDuration = 100;
unsigned int beeps = 2;
unsigned int pauseDuration = 500;
unsigned int cycles = 10;

// Movement sensor
int ultrasonic_1[SENSOR_NUM] = {36, 39};
int ultrasonic_2[SENSOR_NUM] = {34, 35};
float duration_us_1, distance_cm_1;
float duration_us_2, distance_cm_2;
char dis_1[10], dis_2[10];

// Temperature sensor
const int temp = 32;
OneWire oneWire(temp);
DallasTemperature DS18B20(&oneWire);
float current_temp;

// //Light sensor
// BH1750 light_sensor;

// Dimmers
const int zero_cross = 1;
const int dimmer1_pwm = 19;
const int dimmer2_pwm = 18;

// Oled
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

// Functions
void reverse(char *str, int len)
{
  int i = 0, j = len - 1, temp;
  while (i < j)
  {
    temp = str[i];
    str[i] = str[j];
    str[j] = temp;
    i++;
    j--;
  }
}

int intToStr(int x, char str[], int d)
{
  int i = 0;
  while (x)
  {
    str[i++] = (x % 10) + '0';
    x = x / 10;
  }

  while (i < d)
    str[i++] = '0';

  reverse(str, i);
  str[i] = '\0';
  return i;
}

void ftoa(float n, char *res, int afterpoint)
{
  int ipart = (int)n;

  float fpart = n - (float)ipart;

  int i = intToStr(ipart, res, 0);

  if (afterpoint != 0)
  {
    res[i] = '.';

    fpart = fpart * pow(10, afterpoint);

    intToStr((int)fpart, res + i + 1, afterpoint);
  }
}

void read_temp()
{
  DS18B20.requestTemperatures();
  current_temp = DS18B20.getTempCByIndex(0);
  delay(500);
}

void read_key()
{
  char key = keypad.getKey();
  if (key)
  {
    Serial.println(key);
  }
}

void access_granted()
{
  EasyBuzzer.beep(beep_success[0], beep_success[1], beep_success[2], beep_success[3], beep_success[4], beep_success[5]);
  EasyBuzzer.update();
}

void access_denied()
{
  EasyBuzzer.beep(beep_failure[0], beep_failure[1], beep_failure[2], beep_failure[3], beep_failure[4], beep_failure[5]);
  EasyBuzzer.update();
}

void IRAM_ATTR isr()
{
}

void read_distance()
{
  digitalWrite(ultrasonic_1[0], HIGH);
  digitalWrite(ultrasonic_2[0], HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonic_1[0], LOW);
  digitalWrite(ultrasonic_2[0], LOW);
  duration_us_1 = pulseIn(ultrasonic_1[1], HIGH);
  distance_cm_1 = 0.017 * duration_us_1;
  duration_us_2 = pulseIn(ultrasonic_2[1], HIGH);
  distance_cm_2 = 0.017 * duration_us_2;
  Serial.print(distance_cm_1);
  Serial.print(distance_cm_2);
  ftoa(distance_cm_1,dis_1,2);
  ftoa(distance_cm_2,dis_2,2);
  delay(500);
}

void oled()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_ncenB08_tr);
  u8g2.drawStr(0, 10, dis_1);
  u8g2.drawStr(0, 20, dis_2);
  u8g2.sendBuffer();
  delay(500);
}

void setup()
{
  Serial.begin(9600);
  // Oled
  u8g2.begin();
  // Ultrasonic sensors
  pinMode(ultrasonic_1[0], OUTPUT);
  pinMode(ultrasonic_1[1], INPUT);
  pinMode(ultrasonic_2[0], OUTPUT);
  pinMode(ultrasonic_2[1], INPUT);
  // Temp
  // DS18B20.begin();
  // Buzzer
  EasyBuzzer.setPin(buzzer);
}

void loop()
{
  // read_temp();
  access_denied();
  read_key();
  read_distance();
  oled();
}