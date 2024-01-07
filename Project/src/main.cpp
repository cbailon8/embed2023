#include <Arduino.h>
#include <Keypad.h>
#include <OneWire.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <EasyBuzzer.h>
#include <stdio.h>
#include "eepromfn.h"
#include "numtostr.h"
#include "apwifiesp32.h"
#define PRO_CPU 0
#define APP_CPU 1
#define RAM 4096
#define NOAFF_CPU tskNO_AFFINITY
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
const uint8_t ROW_NUM = 4;
const uint8_t COLUMN_NUM = 4;
char keys[ROW_NUM][COLUMN_NUM] = {
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}};
uint8_t pin_rows[ROW_NUM] = {15, 2, 0, 4};
uint8_t pin_column[COLUMN_NUM] = {16, 17, 5, 18};
Keypad keypad = Keypad(makeKeymap(keys), pin_rows, pin_column, ROW_NUM, COLUMN_NUM);
char input_pwd[8];
char contrasena[8] = "1111111";
int keys_pressed = 0;

// Motors
int motor1[ACTUATOR_NUM] = {26, 27};
bool state_motor1 = false;
int motor2[ACTUATOR_NUM] = {25, 33};
bool state_motor2 = false;

// Buzzer
const int buzzer = 12;
bool state_alarm = false;
unsigned int beep_success[2] = {1, 1};
unsigned int beep_failure[2] = {5, 2};

// Movement sensor
const int trigger1 = 5;
const int echo1 = 17;
const int trigger2 = 0;
const int echo2 = 2;
float duration_us_1, duration_us_2;
float distance_cm_1, distance_cm_2 = 0;
char dis_1[10], dis_2[10];

// Temperature sensor
const float ADC_VREF_mV = 3300.0;
const float ADC_RES = 4096.0;
const int temp = 14;
float current_temp;

// //Light sensor
// BH1750 light_sensor;

/*// Dimmers
const int zero_cross = 1;
const int dimmer1_pwm = 19;
bool state_dimm1 = false;
const int dimmer2_pwm = 18;
bool state_dimm2 = false;
dimmerLamp dimm1(dimmer1_pwm, zero_cross);
dimmerLamp dimm2(dimmer2_pwm, zero_cross);
*/

// Oled
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, SCL, SDA, U8X8_PIN_NONE);

// Functions
void read_temp()
{
   int adcVal = analogRead(temp);
  float milivolt = adcVal * (ADC_VREF_mV / ADC_RES);
  current_temp = milivolt / 10;
  Serial.println(current_temp);
  if (current_temp > 25 && !state_motor2)
  {
    Serial.println("ON");
    digitalWrite(motor2[0], HIGH);
    digitalWrite(motor2[1], LOW);
    vTaskDelay(1000);
    digitalWrite(motor2[0], LOW);
    state_motor2 = true;
  }
  else if (current_temp < 25 && state_motor2)
  {
    Serial.println("OFF");
    digitalWrite(motor2[1], HIGH);
    digitalWrite(motor2[0], LOW);
    vTaskDelay(1000);
    digitalWrite(motor2[1], LOW);
    state_motor2 = false;
  }
}

void access_granted()
{
  EasyBuzzer.singleBeep(beep_success[0], beep_success[1]); //, beep_success[2], beep_success[3], beep_success[4], beep_success[5]);
  EasyBuzzer.update();
  digitalWrite(motor1[0], HIGH);
  digitalWrite(motor1[1], LOW);
  delay(1000);
  digitalWrite(motor1[0], LOW);
  state_motor1 = true;
  EasyBuzzer.stopBeep();
}

void access_denied()
{
  EasyBuzzer.singleBeep(beep_failure[0], beep_failure[1]); //, beep_failure[2], beep_failure[3], beep_failure[4], beep_failure[5]);
  EasyBuzzer.update();
  state_alarm = true;
}

/*
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
}*/

void read_distance()
{
  digitalWrite(trigger1, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger1, LOW);
  duration_us_1 = pulseIn(echo1, HIGH);
  distance_cm_1 = 0.017 * duration_us_1;
  digitalWrite(trigger2, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigger2, LOW);
  duration_us_2 = pulseIn(echo2, HIGH);
  distance_cm_2 = 0.017 * duration_us_2;
  Serial.print("Distancia 1 ");
  Serial.println(distance_cm_1);
  Serial.print("Distancia 2 ");
  Serial.println(distance_cm_2);
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

void read_key()
{
  char key = keypad.getKey();
  if (key)
  {
    input_pwd[keys_pressed] = key;
    Serial.println(key);
    keys_pressed = keys_pressed + 1;
  }

  if (keys_pressed >= 7)
  {
    Serial.println("Contrasena:");
    Serial.println(input_pwd);
    if (strcmp(contrasena, input_pwd) == 0)
    {
      Serial.println("Acceso permitido");
      access_granted();
      keys_pressed = 0;
    }
    else
    {
      Serial.println("Acceso denegado");
      access_denied();
      keys_pressed = 0;
    }
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
  pinMode(motor1[0], OUTPUT);
  pinMode(motor1[1], OUTPUT);
  EasyBuzzer.setPin(buzzer);
  read_key();
  EasyBuzzer.setPin(buzzer);
}

void TaskTemp(void *pvParameters)
{
  (void)pvParameters;
  Serial.begin(9600);
  pinMode(motor2[0], OUTPUT);
  pinMode(motor2[1], OUTPUT);
  read_temp();
}

void TaskLights(void *pvParameters)
{
  (void)pvParameters;
  Serial.begin(9600);
  pinMode(trigger1, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(trigger2, OUTPUT);
  pinMode(echo2, INPUT);
  read_distance();
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
  /*if (state_dimm1){
    message += "light";
  }*/
  server.send(200, "text/plain", message);
  vTaskDelay(5000);
}

void loop()
{
}