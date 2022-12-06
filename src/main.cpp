#include <ArduinoJson.h>
#include <M5Core2.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include "M5_ENV.h"

SHT3X sht30;      // 湿温度センサ
QMP6988 qmp6988;  // 気圧センサ

// 関数定義
float getPressure();
float getTemp();
float getHum();
void connectWifi();
void connectAwsIot();
void mqttPublish(const char* topic, char* payload);

// 初期化
float tmp = 0.0;
float hum = 0.0;
float pressure = 0.0;

// 定数
const char* DEVICE_NAME = "env-sensor";
const char* PUBLISH_TOPIC = "env-sensor/publish";
const int AWS_IOT_PORT = 8883;

// env.cpp
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* AWS_IOT_ENDPOINT;

// certs.cpp
extern const char* rootCa;
extern const char* certificate;
extern const char* privateKey;

WiFiClientSecure httpsClient;
PubSubClient mqttClient(httpsClient);

/**
 * 起動時処理
 */
void setup() {
  Serial.begin(115200);
  Serial.println("\nSetup start.");

  M5.begin();

  connectWifi();
  connectAwsIot();

  Wire.begin();    // I2C接続用ライブラリ初期化
  qmp6988.init();  // 気圧センサ初期化

  M5.Lcd.setTextSize(3);
  M5.Lcd.println("Start sending to AWS IoT Core.");
}

/**
 * ループ処理
 */
void loop() {
  DynamicJsonDocument doc(100);

  doc["tmp"] = getTemp();
  doc["hum"] = getHum();
  doc["prs"] = getPressure();

  char json[100];
  serializeJson(doc, json);
  mqttPublish(PUBLISH_TOPIC, json);
  delay(5000);
}

/**
 * WiFi接続
 */
void connectWifi() {
  Serial.println("\nStart WiFi connecting.");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  Serial.println("\nDone!");
}

/**
 * AWS IoT接続
 */
void connectAwsIot() {
  Serial.println("\nStart MQTT setup.");
  // Configure MQTT Client
  httpsClient.setCACert(rootCa);
  httpsClient.setCertificate(certificate);
  httpsClient.setPrivateKey(privateKey);
  mqttClient.setServer(AWS_IOT_ENDPOINT, AWS_IOT_PORT);
  Serial.println("\nDone!");

  // Connect to AWS IoT
  while (!mqttClient.connected()) {
    if (mqttClient.connect(DEVICE_NAME)) {
      Serial.println("Connected.");
    } else {
      Serial.print("Failed. Error state=");
      Serial.print(mqttClient.state());
      delay(5000);
    }
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  // TODO: subscribe処理
}

/**
 * MQTTでトピックにpublish
 */
void mqttPublish(const char* topic, char* payload) {
  Serial.println("\nPublish start.");
  mqttClient.publish(topic, payload);
  Serial.println("\nPublish complate.");
}

/**
 * 気圧取得
 */
float getPressure() { return qmp6988.calcPressure(); }

/**
 * 気温取得
 */
float getTemp() {
  if (sht30.get() == 0) {
    return sht30.cTemp;
  }
  return 0;
}

/**
 * 湿度取得
 */
float getHum() {
  if (sht30.get() == 0) {
    return sht30.humidity;
  }
  return 0;
}