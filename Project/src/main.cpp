#include <Arduino.h>
#include <Keypad.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EasyBuzzer.h>
#define ROW_NUM 4
#define COLUMN_NUM 4
#define SENSOR_NUM 2
#define ACTUATOR_NUM 2

//Keyboard
char keys[ROW_NUM][COLUMN_NUM] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};
byte pin_rows[ROW_NUM] = {16, 4, 0, 2};
byte pin_column[COLUMN_NUM] = {15, 8, 7, 6};
Keypad keypad = Keypad(makeKeymap(keys),pin_rows,pin_column,ROW_NUM,COLUMN_NUM);

//Motors
int motor1[ACTUATOR_NUM] = {25, 26};
int motor2[ACTUATOR_NUM] = {27, 14};

//Buzzer
const int buzzer = 33;
unsigned int beep_success[6] = {1000,50,100,2,500,10};
unsigned int beep_failure[6] = {500,50,100,2,500,10};
unsigned int frequency = 1000;  
unsigned int onDuration = 50;
unsigned int offDuration = 100;
unsigned int beeps = 2;
unsigned int pauseDuration = 500;
unsigned int cycles = 10;

//Movement sensor
int ultrasonic_1[SENSOR_NUM] = {36, 39};
int ultrasonic_2[SENSOR_NUM] = {34, 35};
float duration_us_1, distance_cm_1;
float duration_us_2, distance_cm_2;

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


//Functions
void read_temp() {
  DS18B20.requestTemperatures();
  current_temp = DS18B20.getTempCByIndex(0);
  delay(500);
}

void read_key() {
  char key = keypad.getKey();
  if (key) {
    Serial.println(key);
  }
}

void IRAM_ATTR isr() {

}

void read_distance() {
  digitalWrite(ultrasonic_1[0],HIGH);
  digitalWrite(ultrasonic_2[0],HIGH);
  delayMicroseconds(10);
  digitalWrite(ultrasonic_1[0],LOW);
  digitalWrite(ultrasonic_2[0],LOW);
  duration_us_1 = pulseIn(ultrasonic_1[1],HIGH);
  distance_cm_1 = 0.017*duration_us_1;
  duration_us_2 = pulseIn(ultrasonic_2[1],HIGH);
  distance_cm_2 = 0.017*duration_us_2;
  Serial.print(distance_cm_1);
  Serial.print(distance_cm_2);
  delay(500);
}

void setup() {
  Serial.begin(9600);
  //Ultrasonic sensors
  pinMode(ultrasonic_1[0], OUTPUT);
  pinMode(ultrasonic_1[1], INPUT);
  pinMode(ultrasonic_2[0], OUTPUT);
  pinMode(ultrasonic_2[1], INPUT);
  //Temp
  //DS18B20.begin();
  //Buzzer
  EasyBuzzer.setPin(buzzer);
}

void loop() {
  //read_temp();
  read_key();
}