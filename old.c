/**
    @filename   :   EPD_7in3f.ino
    @brief      :   EPD_7in3 e-paper F display demo
    @author     :   Waveshare

    Copyright (C) Waveshare     10 21 2022

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documnetation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to  whom the Software is
   furished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS OR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.
*/
#include <FS.h>
#include <SPI.h>
#include <math.h>
#include "epd7in3f.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
#include <PubSubClient.h>

#define STB_IMAGE_IMPLEMENTATION

Epd epd;

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }

const char *ssid = "子鸣的 iPhone"; // 请替换为您的WiFi网络名称
const char *password = "lzm040214"; // 请替换为您的WiFi密码
// const char *fileURL = "https://epaper.arunningstar.com/uploads/byte_stream.txt"; // 请替换为要下载的文件URL
const char *filePath = "/byte_stream.txt"; // 下载后保存的文件路径
const char *fileURL = "http://172.20.10.5:5001/uploads/byte_stream.txt";

const char* mqtt_server = "213.202.219.45";
const int mqtt_port = 1883;
const char* mqtt_topic = "byte_stream_topic";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(3000);
  Serial.begin(460800);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.println("File received from MQTT");

  // File file = SPIFFS.open("/received_byte_stream.txt", "w");
  // if (!file) {
  //   Serial.println("Failed to open file for writing");
  //   return;
  // }

  // for (int i = 0; i < length; i++) {
  //   file.write(payload[i]);
  // }

  // file.close();
  // Serial.println("File saved to SPIFFS");
}

void setup()
{
  // if (epd.Init() != 0)
  // {
  //   Serial.print("eP init F");
  //   return;
  // }

  Serial.print("Screen init\n");
  Serial.printf("Startup free size: %d\n", heap_caps_get_free_size(MALLOC_CAP_DEFAULT));

  // epd.EPD_7IN3F_Show7Block();
  // Serial.print("Show pic\r\n ");
  // epd.EPD_7IN3F_Display_part(gImage_7in3f, 250, 150, 300, 180);
  // epd.EPD_7IN3F_Display_part(output_buffer, 0, 120, 800, 240);
  setup_wifi();

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  delay(1000);
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe(mqtt_topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 1 seconds");
      delay(1000);
    }
  }
}

void download_pic()
{    
  HTTPClient http;
  http.begin(fileURL);
  bool recreate_array = false;
  bool already_half = false;
  int httpCode = http.GET();
  if (httpCode == HTTP_CODE_OK)
  {
    u32_t downloaded_bytes = 0;
    WiFiClient *stream = http.getStreamPtr();
    int rate = 0;
    epd.SendCommand(0x10);
    while (downloaded_bytes < 192000)
    {
      uint8_t img_buff[4];

      int bytesRead = stream->readBytes(img_buff, sizeof(img_buff));
      downloaded_bytes += bytesRead;

      if (downloaded_bytes * 100 / 192000 != rate)
      {
        rate = downloaded_bytes * 100 / 192000;
        Serial.printf("%d\n", bytesRead);
        Serial.printf("%d\n", downloaded_bytes);
      }

      if (bytesRead != 0)
      {
        for(int z = 0; z < bytesRead; z++){
          epd.SendData(img_buff[z]);
        }
      }
    }
    epd.TurnOnDisplay();
    delay(1000);
  }
  else
  {
    Serial.printf("HTTP GET failed, error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
}


#define TIME_ADDRESS 0  // 存储时间的起始地址
#define TIME_LENGTH 200 // 存储时间的最大长度，根据需要调整

String readStoredTime()
{
  char storedTime[TIME_LENGTH + 1]; // 创建一个字符数组来存储时间
  EEPROM.begin(TIME_LENGTH + 1);    // 初始化EEPROM库

  // 从EEPROM中读取存储的时间数据
  for (int i = 0; i < TIME_LENGTH; i++)
  {
    storedTime[i] = EEPROM.read(TIME_ADDRESS + i);
  }
  storedTime[TIME_LENGTH] = '\0'; // 添加字符串结束符

  EEPROM.end(); // 结束EEPROM库的使用

  return String(storedTime);
}

void saveTime(String time)
{
  EEPROM.begin(TIME_LENGTH + 1); // 初始化EEPROM库
  int length = time.length() < 200 ? time.length() : 200;

  // 将时间字符串写入EEPROM
  for (int i = 0; i < length; i++)
  {
    EEPROM.write(TIME_ADDRESS + i, time.charAt(i));
  }

  // 如果时间字符串较短，用空字符填充EEPROM余下的空间
  for (int i = length; i < TIME_LENGTH; i++)
  {
    EEPROM.write(TIME_ADDRESS + i, '\0');
  }

  EEPROM.commit(); // 提交更改
  EEPROM.end();    // 结束EEPROM库的使用
}

void checkUpdate()
{

  HTTPClient http;
  // http.begin("http://epaper.arunningstar.com/api/updatetime");
  http.begin("http://172.20.10.5:5001/api/updatetime");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String updateTime = http.getString();
    Serial.println("Remote TIme is: ");
    Serial.println(updateTime);
    String storedTime = readStoredTime();
    Serial.println("Stored TIme is: ");
    Serial.println(storedTime);
    saveTime(updateTime);

    if (updateTime.compareTo(storedTime) > 0)
    {
      // 执行更新操作
      Serial.println("Performing update...");
      // updateEink();
      download_pic();
      // 在这里执行你的更新操作代码

      // 保存最新的更新时间到EEPROM
      saveTime(updateTime);
    }
    else
    {
      Serial.println("No update required.");
    }
  }
  else
  {
    Serial.println("Failed to fetch update time.");
    return;
  }

  http.end();

  String storedTime = readStoredTime();

  // 如果获取的时间比本地时间更新，执行更新操作
}

void loop()
{
  // checkUpdate();

  // delay(5000);
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
}
