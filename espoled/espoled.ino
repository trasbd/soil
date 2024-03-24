#include <U8g2lib.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>

#include "secrets.h"

#define humidity_topic "sensor/humidity"
#define temperature_topic "sensor/temperature"
#define moisture_topic1 "sensor/moisture1"
#define moisture_topic2 "sensor/moisture2"

#define DHTTYPE DHT22
#define DHTPIN D1

const byte numChars = 32;
char receivedChars[numChars];  // an array to store the received data

boolean newData = false;

long lastMsg = 0;
float temp = 0.0;
float hum = 0.0;
float diff = 0.5;

U8G2_SSD1306_128X64_NONAME_F_SW_I2C lcd(U8G2_R0, 14, 12, U8X8_PIN_NONE);

/****** WiFi Connection Details *******/
const char* ssid = SECRET_SSID;
const char* password = SECRET_PASS;

/******* MQTT Broker Connection Details *******/
const char* mqtt_server = SECRET_MQTT_SERVER;
const char* mqtt_username = SECRET_MQTT_USER;
const char* mqtt_password = SECRET_MQTT_PASS;
//const int mqtt_port = 8883;

WiFiClient espClient;
PubSubClient client(espClient);
DHT dht(DHTPIN, DHTTYPE, 11);  // 11 works fine for ESP8266


void setup() {
  Serial.begin(9600);

  lcd.begin();  // initialize the lcd
  // lcd.setBacklight(HIGH);

  lcd.clearBuffer();
  lcd.setFont(u8g2_font_7x13B_mf);
  // u8g2_font_6x13B_mf
  // u8g2_font_7x14B_mf
  lcd.setCursor(0, 10);
  lcd.print("Starting");
  lcd.setCursor(0, 26);
  lcd.print("Please Wait...");
  lcd.sendBuffer();

  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);

  Serial.println("<Arduino is ready>");
}

void loop() {

  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  recvWithEndMarker();
  showNewData();

  long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;

    float newTemp = (dht.readTemperature() * 9.0 / 5.0) + 32.0;
    float newHum = dht.readHumidity();

    if (checkBound(newTemp, temp, diff)) {
      temp = newTemp;
      Serial.print("New temperature:");
      Serial.println(String(temp).c_str());
      client.publish(temperature_topic, String(temp).c_str(), true);
    }

    if (checkBound(newHum, hum, diff)) {
      hum = newHum;
      Serial.print("New humidity:");
      Serial.println(String(hum).c_str());
      client.publish(humidity_topic, String(hum).c_str(), true);
    }
  }
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    // If you do not want to use a username and password, change next line to
    // if (client.connect("ESP8266Client")) {
    if (client.connect("ESP8266Client", mqtt_username, mqtt_password)) {
      Serial.println("connected");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1;
      }
    } else {
      receivedChars[ndx] = '\0';  // terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

void showNewData() {
  if (newData == true) {

    String s = receivedChars;

    int serialInt[2];
    serialInt[0] = s.substring(0, s.indexOf(" ")).toInt();
    serialInt[1] = s.substring(s.indexOf(" ") + 1, s.length()).toInt();

    int serialMax = 1023;
    lcd.setFont(u8g2_font_7x13B_mf);
    lcd.setCursor(0, 10);
    lcd.print(temp);
    lcd.print("\xB0");
    lcd.print("F   ");
    lcd.print(hum);
    lcd.print("%");


    lcd.setFont(u8g2_font_spleen16x32_mf);
    lcd.setCursor(0, 38);
    lcd.print("A1: ");
    lcd.print(100 - serialInt[0] * 100 / serialMax);
    lcd.print("%              ");

    lcd.setCursor(0, 64);
    lcd.print("A2: ");
    lcd.print(100 - serialInt[1] * 100 / serialMax);
    lcd.print("%              ");

    lcd.sendBuffer();

    client.publish(moisture_topic1, String(100 - serialInt[0] * 100 / serialMax).c_str(), true);
    client.publish(moisture_topic2, String(100 - serialInt[1] * 100 / serialMax).c_str(), true);

    newData = false;
  }
}

bool checkBound(float newValue, float prevValue, float maxDiff) {
  return !isnan(newValue) && (newValue < prevValue - maxDiff || newValue > prevValue + maxDiff);
}