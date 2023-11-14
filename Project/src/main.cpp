#include <Arduino.h>
#include <Keypad.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EasyBuzzer.h>
#define ROW_NUM 4
#define COLUMN_NUM 3
#define SENSOR_NUM 2
#define ACTUATOR_NUM 2

//Keyboard
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};
byte pin_rows[ROW_NUM] = {16, 4, 0, 2};
byte pin_column[COLUMN_NUM] = {15, 8, 7};
Keypad keypad = Keypad(makeKeymap(keys),pin_rows,pin_column,ROW_NUM,COLUMN_NUM);

//Motors
int motor1[ACTUATOR_NUM] = {25, 26};
int motor2[ACTUATOR_NUM] = {27, 14};

//Buzzer
const int buzzer = 33;

//Movement sensor
int ultrasonic1[SENSOR_NUM] = {36, 39};
int ultrasonic2[SENSOR_NUM] = {34, 35};

//Temperature sensor
const int temp = 32;
OneWire oneWire(temp);
DallasTemperature DS18B20(&oneWire);
float current_temp;

// //Light sensor
// BH1750 light_sensor;

//Dimmers
const int zero_cross = 1;
const int dimmer1_pwm = 19;
const int dimmer2_pwm = 18;

//Oled

void read_temp() {
  DS18B20.requestTemperatures();
  current_temp = DS18B20.getTempCByIndex(0);
  delay(500);
}

void IRAM_ATTR isr() {

}


void setup() {
  Serial.begin(9600);
  DS18B20.begin();
  EasyBuzzer.setPin(buzzer);
}

void loop() {
  read_temp();
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
  }
}