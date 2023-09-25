#include <FS.h>
#include <SPI.h>
#include <math.h>
#include "epd7in3f.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <esp_wifi.h>

#define STB_IMAGE_IMPLEMENTATION

Epd epd;

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }

extern "C" {
	#include "freertos/FreeRTOS.h"
	#include "freertos/timers.h"
}
#include <AsyncMqttClient.h>

#define WIFI_SSID "tjuwlan"
// #define WIFI_PASSWORD "12345678"
// #define WIFI_SSID "子鸣的 iPhone"
// #define WIFI_PASSWORD "lzm040214"
#define MQTT_PORT 1883

AsyncMqttClient mqttClient;
WiFiClientSecure espClient;

TimerHandle_t mqttReconnectTimer;
TimerHandle_t wifiReconnectTimer;

void connectToWifi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.begin(WIFI_SSID, "");
}

WiFiClientSecure net;

void connectToMqtt() {
  Serial.println("Connecting to MQTT...");
  // 设置其他MQTT参数
  mqttClient.setServer("213.202.219.45", 1883); // 替换为您的MQTT代理地址和端口
  mqttClient.setCredentials("esp32", "12345678"); // 替换为您的MQTT代理用户名和密码
  mqttClient.setKeepAlive(300);
  mqttClient.connect();
}

void WiFiEvent(WiFiEvent_t event) {
    Serial.printf("[WiFi-event] event: %d\n", event);
    switch(event) {
    case SYSTEM_EVENT_STA_GOT_IP:
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        connectToMqtt();
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        Serial.println("WiFi lost connection");
        xTimerStop(mqttReconnectTimer, 0); // ensure we don't reconnect to MQTT while reconnecting to Wi-Fi
        xTimerStart(wifiReconnectTimer, 0);
        break;
    }
}

void onMqttConnect(bool sessionPresent) {
  Serial.println("Connected to MQTT.");
  Serial.print("Session present: ");
  Serial.println(sessionPresent);
  uint16_t packetIdSub = mqttClient.subscribe("lazys_epaper_byte_stream", 2);
  Serial.print("Subscribing at QoS 0, packetId: ");
  Serial.println(packetIdSub);
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");
  if (WiFi.isConnected()) {
    xTimerStart(mqttReconnectTimer, 0);
  }
}

void onMqttSubscribe(uint16_t packetId, uint8_t qos) {
  Serial.println("Subscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
  Serial.print("  qos: ");
  Serial.println(qos);
}

void onMqttUnsubscribe(uint16_t packetId) {
  Serial.println("Unsubscribe acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

int rate = 0;

void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
  // Serial.println("Publish received.");
  // Serial.print("  topic: ");
  // Serial.println(topic);
  // Serial.print("  qos: ");
  // Serial.println(properties.qos);
  // Serial.print("  dup: ");
  // Serial.println(properties.dup);
  // Serial.print("  retain: ");
  // Serial.println(properties.retain);
  // Serial.print("  len: ");
  // Serial.println(len);
  // Serial.print("  index: ");
  // Serial.println(index);
  // Serial.print("  total: ");
  // Serial.println(total);
  // Serial.print(payload);
  if(index == 0){
    Serial.println("接收到新图片");
    // epd.SendCommand(0x10);
  }
  if((index + len) * 100 / 192000 != rate){
      rate = (index + len) * 100 / 192000;
      Serial.print("图片接收进度：");
      Serial.println(rate);
   }
  // for(int i = 0; i < len; i++){
  //   epd.SendData(payload[i]);
    
  // }
  if(index + len == 192000){
    Serial.println("图片接收完成");

    // epd.TurnOnDisplay();
    rate = 0;
  }
}

void onMqttPublish(uint16_t packetId) {
  Serial.println("Publish acknowledged.");
  Serial.print("  packetId: ");
  Serial.println(packetId);
}

void setup() {
  // f8:4d:89:66:4d:c2
  Serial.begin(115200);
  delay(3000);
  uint8_t newMACAddress[] = {0xF8, 0x4D, 0x89, 0x66, 0x4D, 0xC2};
  WiFi.mode(WIFI_STA);
  esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]);
  Serial.print("[NEW] ESP32 Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  // if (epd.Init() != 0)
  // {
  //   Serial.print("eP init F");
  //   return;
  // }

  Serial.print("Screen init\n");
  Serial.printf("Startup free size: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

  mqttReconnectTimer = xTimerCreate("mqttTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToMqtt));
  wifiReconnectTimer = xTimerCreate("wifiTimer", pdMS_TO_TICKS(2000), pdFALSE, (void*)0, reinterpret_cast<TimerCallbackFunction_t>(connectToWifi));

  WiFi.onEvent(WiFiEvent);

  mqttClient.onConnect(onMqttConnect);
  mqttClient.onDisconnect(onMqttDisconnect);
  mqttClient.onSubscribe(onMqttSubscribe);
  mqttClient.onUnsubscribe(onMqttUnsubscribe);
  mqttClient.onMessage(onMqttMessage);
  // mqttClient.onPublish(onMqttPublish);
  connectToWifi();
  uint8_t mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC Address: ");
  Serial.print(mac[0], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.println(mac[5], HEX);
}

void loop() {
}