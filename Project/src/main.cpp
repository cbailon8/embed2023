#include <Arduino.h>
#include <Keypad.h>
#include <BH1750.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EasyBuzzer.h>
#include <stdio.h>
#include <RBDDimmer.h>
#include "eepromfn.h"
#include "numtostr.h"
#include "apwifiesp32.h"
#define PRO_CPU 0
#define APP_CPU 1
#define RAM 4096
#define NOAFF_CPU tskNO_AFFINITY
#define ROW_NUM 4
#define COLUMN_NUM 4
#define SENSOR_NUM 2
#define ACTUATOR_NUM 2
#define SCL 22
#define SDA 21
#define PWD_ADDR 16

// Connection
String ssid = "12345678";
String password = "12345678";

// Tasks
void TaskLights(void *pvParameters);
void TaskTemp(void *pvParameters);
void TaskAccess(void *pvParameters);
void TaskScreen(void *pvParameters);
void TaskAccessPoint(void *pvParameters);
void TaskSendParams(void *pvParameters);

// Keyboard
char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
byte pin_rows[ROW_NUM] = {36, 39, 34, 35};
byte pin_column[COLUMN_NUM] = {32, 33, 25, 26};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
char input_pwd[8];

// Motors
int motor1[ACTUATOR_NUM] = {13, 9};
bool state_motor1 = false;
int motor2[ACTUATOR_NUM] = {10, 11};
bool state_motor2 = false;

// Buzzer
const int buzzer = 12;
bool state_alarm = false;
unsigned int beep_success[6] = {1000, 50, 100, 2, 500, 10};
unsigned int beep_failure[6] = {500, 50, 100, 2, 500, 10};
unsigned int frequency = 1000;
unsigned int onDuration = 50;
unsigned int offDuration = 100;
unsigned int beeps = 2;
unsigned int pauseDuration = 500;
unsigned int cycles = 10;

// Movement sensor
int ultrasonic_1[SENSOR_NUM] = {15, 8};
int ultrasonic_2[SENSOR_NUM] = {7, 6};
float duration_us_1, duration_us_2;
float distance_cm_1, distance_cm_2 = 0;
char dis_1[10], dis_2[10];

// Temperature sensor
const int temp = 14;
OneWire oneWire(temp);
DallasTemperature DS18B20(&oneWire);
float current_temp;

// //Light sensor
// BH1750 light_sensor;

// Dimmers
const int zero_cross = 1;
const int dimmer1_pwm = 19;
bool state_dimm1 = false;
const int dimmer2_pwm = 18;
bool state_dimm2 = false;
dimmerLamp dimm1(dimmer1_pwm, zero_cross);
dimmerLamp dimm2(dimmer2_pwm, zero_cross);

// Oled
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

// Functions
void read_temp()
{
  DS18B20.requestTemperatures();
  current_temp = DS18B20.getTempCByIndex(0);
  Serial.print(current_temp);
  if (current_temp > 25)
  {
    digitalWrite(motor1[0], HIGH);
    state_motor1 = true;
    vTaskDelay(5000);
    digitalWrite(motor1[0], LOW);
    state_motor1 = true;
  }
  else if (current_temp < 25)
  {
    digitalWrite(motor1[1], HIGH);
    state_motor1 = true;
    vTaskDelay(5000);
    digitalWrite(motor1[1], LOW);
    state_motor1 = true;
  }
  vTaskDelay(500);
}

void read_key()
{
  int keys_pressed = 0;
  char key = keypad.getKey();
  if (key)
  {
    Serial.println(key);
    while (keys_pressed < 8)
    {
      input_pwd[keys_pressed++] = key;
    }
    keys_pressed = 0;
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
  state_alarm = true;
}

void manage_lights()
{
  if (distance_cm_1 < 10)
  {
    dimm1.setPower(80);
  }
  else
  {
    dimm1.setPower(0);
  }
  if (distance_cm_2 < 10)
  {
    dimm2.setPower(80);
  }
  else
  {
    dimm2.setPower(0);
  }
}

void read_distance()
{
  digitalWrite(ultrasonic_1[0], HIGH);
  digitalWrite(ultrasonic_2[0], HIGH);
  vTaskDelay(1);
  digitalWrite(ultrasonic_1[0], LOW);
  digitalWrite(ultrasonic_2[0], LOW);
  duration_us_1 = pulseIn(ultrasonic_1[1], HIGH);
  distance_cm_1 = 0.017 * duration_us_1;
  duration_us_2 = pulseIn(ultrasonic_2[1], HIGH);
  distance_cm_2 = 0.017 * duration_us_2;
  Serial.print(distance_cm_1);
  Serial.print(distance_cm_2);
  ftoa(distance_cm_1, dis_1, 2);
  ftoa(distance_cm_2, dis_2, 2);
  vTaskDelay(500);
}

void oled()
{
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_tiny5_tf);
  u8g2.drawStr(0, 6, "Distancia:");
  u8g2.drawStr(20, 6, dis_1);
  u8g2.drawStr(0, 12, "Distancia:");
  u8g2.drawStr(20, 12, dis_2);
  if (state_alarm)
  {
    u8g2.drawStr(0, 18, "¡Alarma activada!");
  }
  if (state_motor1)
  {
    u8g2.drawStr(0, 24, "Puerta: \"ON\"");
  }
  else
  {
    u8g2.drawStr(0, 24, "Puerta: \"OFF\"");
  }
  if (state_motor2)
  {
    u8g2.drawStr(0, 30, "Persiana: \"ON\"");
  }
  else
  {
    u8g2.drawStr(0, 24, "Persiana: \"OFF\"");
  }
  u8g2.sendBuffer();
  vTaskDelay(500);
}

void check_pwd()
{
  char pwd[] = "";
  int comp = strcmp(pwd, input_pwd);
  if (comp == 0)
  {
    access_granted();
  }
  else
  {
    access_denied();
  }
}

void setup()
{
  EEPROM.begin(512);
  writeStringEEPROM("ABC123", PWD_ADDR);
  initAP(ssid.c_str(), password.c_str());
  xTaskCreatePinnedToCore(TaskAccess, "TaskAccess", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskLights, "TaskLights", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskTemp, "TaskTemp", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskScreen, "TaskScreen", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskAccessPoint, "TaskAccessPoint", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskSendParams, "TaskSendParams", RAM, NULL, 1, NULL, APP_CPU);
}

void TaskAccess(void *pvParameters)
{
  (void)pvParameters;
  Serial.begin(9600);
  read_key();
  check_pwd();
  EasyBuzzer.setPin(buzzer);
}

void TaskTemp(void *pvParameters)
{
  (void)pvParameters;
  Serial.begin(9600);
  // DS18B20.begin();
  // read_temp();
}

void TaskLights(void *pvParameters)
{
  (void)pvParameters;
  Serial.begin(9600);
  pinMode(ultrasonic_1[0], OUTPUT);
  pinMode(ultrasonic_1[1], INPUT);
  pinMode(ultrasonic_2[0], OUTPUT);
  pinMode(ultrasonic_2[1], INPUT);
  read_distance();
  manage_lights();
}

void TaskScreen(void *pvParameters)
{
  (void)pvParameters;
  // u8g2.begin();
  // oled();
}

void TaskAccessPoint(void *pvParameters)
{
  (void)pvParameters;
  loopAP();
}

void TaskSendParams(void *pvParameters){
  (void)pvParameters;
  String message = "";
  if (state_motor1){
    message += "door";
  }
  if (state_motor2){
    message += "window";
  }
  if (state_dimm1){
    message += "light";
  }
  server.send(200, "text/plain", message);
  vTaskDelay(5000);
}

void loop()
{
}