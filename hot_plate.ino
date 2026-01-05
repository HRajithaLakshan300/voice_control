#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <max6675.h>

/* ---------- WIFI ---------- */
const char* ssid = "username";
const char* password = "passworld";

/* ---------- MQTT ---------- */
const char* mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
const char* topicSetTemp = "smdhotplate/setTemp";
const char* topicCurrentTemp = "smdhotplate/currentTemp";

/* ---------- LCD ---------- */
LiquidCrystal_PCF8574 lcd(0x27);

/* ---------- MAX6675 ---------- */
int thermoSO = 19;
int thermoCS = 5;
int thermoSCK = 18;
MAX6675 thermocouple(thermoSCK, thermoCS, thermoSO);

/* ---------- SSR ---------- */
#define SSR_PIN 26

float targetTemp = 180.0;
float hysteresis = 3.0;

WiFiClient espClient;
PubSubClient client(espClient);

/* ---------- MQTT CALLBACK ---------- */
void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  float newTemp = atof((char*)payload);

  if (newTemp >= 50 && newTemp <= 250) {
    targetTemp = newTemp;
    Serial.print("Target Temp Updated: ");
    Serial.println(targetTemp);
  }
}

/* ---------- MQTT CONNECT ---------- */
void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32_SMD_HOTPLATE")) {
      Serial.println("Connected");
      client.subscribe(topicSetTemp);
    } else {
      Serial.println("Failed, retrying...");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.print("SMD HOT PLATE");

  pinMode(SSR_PIN, OUTPUT);
  digitalWrite(SSR_PIN, LOW);

  /* ---------- WIFI ---------- */
  WiFi.begin(ssid, password);
  Serial.print("Connecting WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  /* ---------- MQTT ---------- */
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  float tempC = thermocouple.readCelsius();

  /* ---------- HEATER CONTROL ---------- */
  if (tempC < (targetTemp - hysteresis)) {
    digitalWrite(SSR_PIN, HIGH);
  } else if (tempC > (targetTemp + hysteresis)) {
    digitalWrite(SSR_PIN, LOW);
  }

  /* ---------- LCD ---------- */
  lcd.setCursor(0, 0);
  lcd.print("T:");
  lcd.print(tempC, 1);
  lcd.print("C   ");

  lcd.setCursor(0, 1);
  lcd.print("Set:");
  lcd.print(targetTemp, 0);
  lcd.print("C   ");

  /* ---------- MQTT PUBLISH ---------- */
  char tempStr[10];
  dtostrf(tempC, 4, 1, tempStr);
  client.publish(topicCurrentTemp, tempStr);

  delay(1000);
}
