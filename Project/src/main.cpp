#include <Arduino.h>
#include <Keypad.h>
#include <SPI.h>
#include <WiFi.h>
#include <WebServer.h>
#include <EasyBuzzer.h>
#include <stdio.h>
#include "eepromfn.h"
#include "numtostr.h"
#include <UbidotsEsp32Mqtt.h>
#define PRO_CPU 0
#define APP_CPU 1
#define RAM 16384
#define NOAFF_CPU tskNO_AFFINITY
#define SENSOR_NUM 2
#define ACTUATOR_NUM 2
#define SCL 22
#define SDA 21
#define PWD_ADDR 16

// Connection
const char *ssid = "HOME-ESP32";
const char *password = "12345678";
WebServer server(80);

// Tasks
void TaskLights(void *pvParameters);
void TaskTemp(void *pvParameters);
void TaskAccess(void *pvParameters);
void TaskScreen(void *pvParameters);
void TaskAccessPoint(void *pvParameters);
void TaskSendParams(void *pvParameters);
void TaskUbidots(void *pvParameters);

// Ubidots
const char *token = "BBUS-OpLOQiKcJwiFKD8bqmyRD4qlU3pYgb";
const char *device = "esp32";
const char *var1 = "temperature";
const char *var2 = "door";
const char *var3 = "window";
const char *var4 = "light1";
const char *var5 = "light2";
const int publish_frequency = 60 * 1000;
unsigned long timer;
Ubidots ubidots(token);

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
const int trigger2 = 21;
const int echo2 = 19;
const int trigger1 = 23;
const int echo1 = 22;
float duration_us_1, duration_us_2;
float distance_cm_1, distance_cm_2 = 0;
char dis_1[10], dis_2[10];

// Temperature sensor
const float ADC_VREF_mV = 3300.0;
const float ADC_RES = 4096.0;
const int temp = 14;
float current_temp;

// Lights
bool state_led1 = false;
bool state_led2 = false;
const int led1 = 13;
const int led2 = 32;

// Functions

void payload()
{
}

void read_temp()
{
  int adcVal = analogRead(temp);
  float milivolt = adcVal * (ADC_VREF_mV / ADC_RES);
  current_temp = milivolt / 10;
  Serial.print("Temperatura: ");
  Serial.println(current_temp);
  if (current_temp > 25 && !state_motor2)
  {
    Serial.println("ON");
    digitalWrite(motor2[0], HIGH);
    digitalWrite(motor2[1], LOW);
    vTaskDelay(3 * 1000);
    digitalWrite(motor2[0], LOW);
    state_motor2 = true;
  }
  else if (current_temp < 25 && state_motor2)
  {
    Serial.println("OFF");
    digitalWrite(motor2[1], HIGH);
    digitalWrite(motor2[0], LOW);
    vTaskDelay(3 * 1000);
    digitalWrite(motor2[1], LOW);
    state_motor2 = false;
  }
};

void access_granted()
{
  EasyBuzzer.singleBeep(beep_success[0], beep_success[1]);
  EasyBuzzer.update();
  digitalWrite(motor1[0], HIGH);
  digitalWrite(motor1[1], LOW);
  vTaskDelay(3 * 1000);
  digitalWrite(motor1[0], LOW);
  state_motor1 = true;
  EasyBuzzer.stopBeep();
  state_motor1 = false;
};

void access_denied()
{
  EasyBuzzer.singleBeep(beep_failure[0], beep_failure[1]);
  EasyBuzzer.update();
  state_alarm = true;
};

void manage_lights()
{
  if (distance_cm_1 < 10)
  {
    digitalWrite(led1, HIGH);
    Serial.println("LED 1 ON");
    state_led1 = true;
  }
  else
  {
    digitalWrite(led1, LOW);
    Serial.println("LED 1 OFF");
    state_led1 = false;
  }
  if (distance_cm_2 < 10)
  {
    digitalWrite(led2, HIGH);
    Serial.println("LED 2 ON");
    state_led2 = true;
  }
  else
  {
    digitalWrite(led2, LOW);
    Serial.println("LED 2 OFF");
    state_led2 = false;
  }
}

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
  Serial.print("Distancia 1: ");
  Serial.println(distance_cm_1);
  Serial.print("Distancia 2: ");
  Serial.println(distance_cm_2);
  ftoa(distance_cm_1, dis_1, 2);
  ftoa(distance_cm_2, dis_2, 2);
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

void handleRoot()
{
  String html = "<html><body>";
  html += "<form method='POST' action='/home'>";
  html += "Luces: <input type='text' name='light'><br>";
  html += "Puerta: <input type='text' name='door'><br>";
  html += "Persianas: <input type='text' name='window'><br>";
  html += "<input type='submit' value='Enviar'>";
  html += "</form></body></html>";
  server.send(200, "text/html", html);
}

void handleHome()
{
  if (strcmp("true", server.arg("light").c_str()) == 0)
  {
    Serial.println("Luz");
    state_led1 = true;
    state_led2 = true;
    digitalWrite(led1, HIGH);
    digitalWrite(led2, HIGH);
  }
  else
  {
    state_led1 = false;
    state_led2 = false;
    digitalWrite(led1, LOW);
    digitalWrite(led2, LOW);
  }
  if (strcmp("true", server.arg("door").c_str()) == 0)
  {
    Serial.println("Puerta");
    access_granted();
  }
  if (strcmp("true", server.arg("window").c_str()) == 0)
  {
    Serial.println("Ventana");
    Serial.println("ON");
    digitalWrite(motor2[0], HIGH);
    digitalWrite(motor2[1], LOW);
    vTaskDelay(3 * 1000);
    digitalWrite(motor2[0], LOW);
    state_motor2 = true;
  }
  else if (strcmp("false", server.arg("window").c_str()) == 0)
  {
    Serial.println("OFF");
    digitalWrite(motor2[1], HIGH);
    digitalWrite(motor2[0], LOW);
    vTaskDelay(3 * 1000);
    digitalWrite(motor2[1], LOW);
    state_motor2 = false;
  }
}

void initAP(const char *apSsid, const char *apPassword)
{ // Nombre de la red Wi-Fi y  Contraseña creada por el ESP32
  Serial.begin(115200);

  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid, apPassword);

  server.on("/", handleRoot);
  server.on("/home", handleHome);

  server.begin();
  Serial.print("Ip de esp32...");
  Serial.println(WiFi.softAPIP());
  Serial.println("Servidor web iniciado");
}

void setup()
{
  Serial.begin(115200);
  initAP(ssid, password);
  xTaskCreatePinnedToCore(TaskAccess, "TaskAccess", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskLights, "TaskLights", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskTemp, "TaskTemp", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskAccessPoint, "TaskAccessPoint", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskSendParams, "TaskSendParams", RAM, NULL, 1, NULL, APP_CPU);
  xTaskCreatePinnedToCore(TaskUbidots, "TaskUbidots", RAM, NULL, 1, NULL, APP_CPU);
}

void TaskAccess(void *pvParameters)
{
  (void)pvParameters;
  pinMode(motor1[0], OUTPUT);
  pinMode(motor1[1], OUTPUT);
  EasyBuzzer.setPin(buzzer);
  while (1)
  {
    read_key();
  }
}

void TaskTemp(void *pvParameters)
{
  (void)pvParameters;
  pinMode(motor2[0], OUTPUT);
  pinMode(motor2[1], OUTPUT);
  while (1)
  {
    read_temp();
    vTaskDelay(30 * 1000);
  }
}

void TaskLights(void *pvParameters)
{
  (void)pvParameters;
  pinMode(trigger1, OUTPUT);
  pinMode(echo1, INPUT);
  pinMode(trigger2, OUTPUT);
  pinMode(echo2, INPUT);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  while (1)
  {
    read_distance();
    manage_lights();
    vTaskDelay(10 * 1000);
  }
}

void TaskUbidots(void *pvParameters)
{
  (void)pvParameters;
  ubidots.connectToWifi("R12C", "12345678");
  ubidots.setup();
  ubidots.reconnect();
  timer = millis();
  while (1)
  {
    if (!ubidots.connected())
    {
      ubidots.reconnect();
    }
    unsigned long now = millis() - timer;
    if (abs(int(now)) > publish_frequency)
    {
      ubidots.add(var1, current_temp);
      ubidots.add(var2, state_motor1);
      ubidots.add(var3, state_motor2);
      ubidots.add(var4, state_led1);
      ubidots.add(var5, state_led2);
      ubidots.publish(device);
      timer = millis();
    }
    ubidots.loop();
  }
}

void TaskAccessPoint(void *pvParameters)
{
  (void)pvParameters;
  while (1)
  {
    server.handleClient();
  }
}

void TaskSendParams(void *pvParameters)
{
  (void)pvParameters;
  String message = "";
  while (1)
  {
    if (state_motor1)
    {
      message += "door";
    }
    if (state_motor2)
    {
      message += "window";
    }
    if (state_led1 || state_led2)
    {
      message += "light";
    }
    server.send(200, "text/plain", message);
    message = "";
  }
  vTaskDelay(20 * 1000);
}

void loop()
{
}