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

#include <SPI.h>
#include <math.h>

#include "epd7in3f.h"
#include "LittleFS.h"
#include <AutoWifi.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>

#define STB_IMAGE_IMPLEMENTATION

Epd epd;

uint16_t width() { return EPD_WIDTH; }
uint16_t height() { return EPD_HEIGHT; }

// uint8_t output_buffer[EPD_WIDTH * EPD_HEIGHT / 4];

// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f)
{
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f)
{
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

void setup()
{
  AutoWifi a;
  a.startWifi();
  // put your setup code here, to run once:
  Serial.begin(115200);

  delay(3000);
  Serial.print("Serial Init Done\n");

  if (epd.Init() != 0)
  {
    Serial.print("eP init F");
    return;
  }

  // epd.Clear(EPD_7IN3F_WHITE);

  // Serial.print("Show pic\r\n ");
  // epd.EPD_7IN3F_Display_part(large_img, 0, 0, 800, 480);

  epd.Sleep();
}

void updateEink()
{
  epd.TurnOnDisplay();
  HTTPClient http;
  http.begin("http://192.168.1.196:5001/api/display");
  int httpCode = http.GET();
  if (httpCode > 0)
  {
    if (httpCode == HTTP_CODE_OK)
    {
      unsigned char* dataArray = new unsigned char[192000];
      delay(500);

      Serial.print("开始读取字节流\n");
      int len = http.getSize();
      // create buffer for read
      Serial.print(len);
      Serial.print('\n');

      // get tcp stream
      WiFiClient *stream = http.getStreamPtr();
      for (int i = 0; i < 300; i++)
      {
        unsigned char* buff = new unsigned char[640];
        stream->readBytes(buff, 640);
        memcpy(dataArray + i * 640, buff, 640);
        delay(20);
        Serial.print(i);
        delete[] buff;
      }
      delay(500);
      Serial.print("\n字节流读取完毕\n");
      // const unsigned gImage_7in3f[192000] = dataArray;
      // for(int k = 0; k < 192000; k++){
      //   Serial.print(dataArray[k]);
      // }
      epd.EPD_7IN3F_Display_part(dataArray, 0, 0, 800, 480);
      delay(500);

      Serial.print("屏幕刷新完成\n");
      epd.Sleep();

      delete[] dataArray;
    }
  }
  http.end();

}

#define TIME_ADDRESS 0 // 存储时间的起始地址
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
  http.begin("http://192.168.1.196:5001/api/updatetime");
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK)
  {
    String updateTime = http.getString();
    Serial.println("Remote TIme is: ");
    Serial.println(updateTime);
    String storedTime = readStoredTime();
    Serial.println("Stored TIme is: ");
    Serial.println(storedTime);

    if (updateTime.compareTo(storedTime) > 0)
    {
      // 执行更新操作
      Serial.println("Performing update...");
      updateEink();
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
  checkUpdate();

  delay(5000);
}
