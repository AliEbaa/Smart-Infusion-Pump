/*************************************************
 * SMART INFUSION PUMP SYSTEM
 * ESP32 DevKitC
 * Version Final
 *************************************************/

/******************** LIBRARIES ********************/
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_INA219.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "HX711.h"

/******************** WIFI + MQTT ********************/
const char* ssid = "YOUR_WIFI";
const char* password = "YOUR_PASSWORD";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);

/******************** PIN ASSIGNMENTS ********************/

#define FLOW_SENSOR   14

#define PUMP_PIN      16

#define HX_DT         18
#define HX_SCK        19

#define SDA_PIN       21
#define SCL_PIN       22

#define BUZZER        4
#define LED_ALARM     25

#define BTN_UP        26
#define BTN_DOWN      27

#define BTN_PRIMING   32
#define BTN_INFUSION  33

/******************** OLED ********************/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  -1
);

/******************** OBJECTS ********************/
Adafruit_INA219 ina219;
HX711 scale;

/******************** FLOW SENSOR ********************/
volatile unsigned long flow_pulses = 0;

float flow_rate = 0.0;
float total_volume = 0.0;

/******************** MOVING AVERAGE ********************/
#define FLOW_SAMPLES 5

float flow_buffer[FLOW_SAMPLES] = {0};
int flow_index = 0;

/******************** SENSOR VALUES ********************/
float current_mA = 0;
float weight = 0;

/******************** USER SETTINGS ********************/
float set_rate = 10.0;
float target_volume = 50.0;

/******************** PID ********************/
float Kp = 5.0;
float Ki = 0.3;
float Kd = 1.0;

float error = 0;
float prev_error = 0;

float integral = 0;
float output_pwm = 0;

#define INTEGRAL_MAX 100
#define INTEGRAL_MIN -100

/******************** STATE MACHINE ********************/
enum State
{
  IDLE,
  PRIMING,
  INFUSION,
  COMPLETE,
  ALARM
};

State state = IDLE;

/******************** TIMERS ********************/
unsigned long t_flow = 0;
unsigned long t_sensor = 0;
unsigned long t_display = 0;
unsigned long t_mqtt = 0;
unsigned long t_pid = 0;

unsigned long priming_start = 0;

/******************** ALARM FLAG ********************/
bool alarmStarted = false;
/******************** INTERRUPT ********************/
void IRAM_ATTR flowISR()
{
  flow_pulses++;
}

/******************** WIFI ********************/
void setup_wifi()
{
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(300);
  }
}

/******************** MQTT ********************/
void reconnectMQTT()
{
  static unsigned long lastTry = 0;

  if (client.connected())
    return;

  if (millis() - lastTry > 5000)
  {
    client.connect("ESP32_INFUSION");
    lastTry = millis();
  }
}

void sendMQTT()
{
  char msg[50];

  sprintf(msg, "%.2f", flow_rate);
  client.publish("infusion/flow", msg);

  sprintf(msg, "%.2f", total_volume);
  client.publish("infusion/volume", msg);

  sprintf(msg, "%.2f", current_mA);
  client.publish("infusion/current", msg);

  sprintf(msg, "%.2f", weight);
  client.publish("infusion/weight", msg);
}
void setup()
{
  Serial.begin(115200);
  delay(1000);  
  pinMode(LED_ALARM, OUTPUT);
  pinMode(BUZZER, OUTPUT);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);

  pinMode(BTN_PRIMING, INPUT_PULLUP);
  pinMode(BTN_INFUSION, INPUT_PULLUP);

  pinMode(FLOW_SENSOR, INPUT_PULLUP);

  attachInterrupt(
    digitalPinToInterrupt(FLOW_SENSOR),
    flowISR,
    RISING
  );

  Wire.begin(SDA_PIN, SCL_PIN);

  ina219.begin();

  display.begin(
    SSD1306_SWITCHCAPVCC,
    0x3C
  );

  display.clearDisplay();
  display.display();

  scale.begin(HX_DT, HX_SCK);

  scale.set_scale(420.0);
  scale.tare();

  ledcAttach(PUMP_PIN, 5000, 8);

  setup_wifi();

  client.setServer(mqtt_server, 1883);
}
void readSensors()
{
  current_mA = ina219.getCurrent_mA();

  weight = scale.get_units(10);
}

/******************** FLOW SENSOR ********************/
void readFlow()
{
  if (millis() - t_flow >= 200)
  {
    noInterrupts();

    unsigned long pulses = flow_pulses;

    flow_pulses = 0;

    interrupts();

    float frequency = pulses * 5.0;

    float instantFlow = frequency / 7.5;

    flow_buffer[flow_index] = instantFlow;

    flow_index =
      (flow_index + 1) % FLOW_SAMPLES;

    float sum = 0;

    for (int i = 0; i < FLOW_SAMPLES; i++)
    {
      sum += flow_buffer[i];
    }

    flow_rate = sum / FLOW_SAMPLES;

    total_volume += flow_rate / 300.0;

    t_flow = millis();
  }
}
void runPID()
{
  if (millis() - t_pid >= 200)
  {
    error = set_rate - flow_rate;

    integral += error * 0.2;

    integral = constrain(
      integral,
      INTEGRAL_MIN,
      INTEGRAL_MAX
    );

    float derivative =
      (error - prev_error) / 0.2;

    output_pwm =
      (Kp * error)
      + (Ki * integral)
      + (Kd * derivative);

    output_pwm =
      constrain(output_pwm, 0, 255);

    ledcWrite(PUMP_PIN, output_pwm);

    prev_error = error;

    t_pid = millis();
  }
}
void handleButtons()
{
  static unsigned long lastPress = 0;

  if (millis() - lastPress < 200)
    return;

  if (digitalRead(BTN_UP) == LOW)
  {
    set_rate += 1;
    lastPress = millis();
  }

  if (digitalRead(BTN_DOWN) == LOW)
  {
    if (set_rate > 1)
      set_rate -= 1;

    lastPress = millis();
  }
}

/******************** STATE BUTTONS ********************/
void handleStateButtons()
{
  static unsigned long lastPress = 0;

  if (millis() - lastPress < 300)
    return;

  if (digitalRead(BTN_PRIMING) == LOW)
  {
    if (state == IDLE)
    {
      priming_start = millis();
      state = PRIMING;
    }

    lastPress = millis();
  }

  if (digitalRead(BTN_INFUSION) == LOW)
  {
    if (state == IDLE)
    {
      integral = 0;
      prev_error = 0;

      state = INFUSION;
    }

    lastPress = millis();
  }
}
void checkSafety()
{
  if (current_mA > 800 &&
      flow_rate < 1)
  {
    state = ALARM;
  }

  if (weight < 5)
  {
    state = ALARM;
  }

  if (total_volume >= target_volume)
  {
    state = COMPLETE;
  }
}
void runPriming()
{
  ledcWrite(PUMP_PIN,180);

  if (flow_rate > 5)
  {
    state = IDLE;
  }

  if (millis() - priming_start > 5000)
  {
    state = IDLE;
  }
}

void updateDisplay()
{
  if (millis() - t_display < 1000)
    return;

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0,0);
  display.print("Rate:");
  display.print(set_rate);

  display.setCursor(0,10);
  display.print("Flow:");
  display.print(flow_rate);

  display.setCursor(0,20);
  display.print("Vol:");
  display.print(total_volume);

  display.setCursor(0,30);
  display.print("W:");
  display.print(weight);

  display.setCursor(0,40);
  display.print("Cur:");
  display.print(current_mA);

  display.setCursor(0,55);

  switch(state)
  {
    case IDLE:
      display.print("IDLE");
      break;

    case PRIMING:
      display.print("PRIMING");
      break;

    case INFUSION:
      display.print("INFUSION");
      break;

    case COMPLETE:
      display.print("COMPLETE");
      break;

    case ALARM:
      display.print("ALARM");
      break;
  }

  display.display();

  t_display = millis();
}
void loop()
{
  digitalWrite(LED_ALARM, LOW);

  if(state != ALARM)
  {
    noTone(BUZZER);
    alarmStarted = false;
  }

  handleButtons();

  handleStateButtons();

  readFlow();

  if (millis() - t_sensor >= 200)
  {
    readSensors();
    t_sensor = millis();
  }

  reconnectMQTT();

  client.loop();

  if (millis() - t_mqtt >= 2000)
  {
    sendMQTT();
    t_mqtt = millis();
  }

  checkSafety();

  switch(state)
  {
    case IDLE:

      ledcWrite(PUMP_PIN,0);

      break;

    case PRIMING:

      runPriming();

      break;

    case INFUSION:

      runPID();

      break;

    case COMPLETE:

      ledcWrite(0, 0);

      digitalWrite(
        LED_ALARM,
        HIGH
      );

      tone(
        BUZZER,
        1500,
        500
      );

      break;

    case ALARM:

      ledcWrite(0, 0);

      digitalWrite(
        LED_ALARM,
        HIGH
      );

      if(!alarmStarted)
      {
        tone(BUZZER, 2000);
        alarmStarted = true;
      }

      break;
  }

  updateDisplay();
}
