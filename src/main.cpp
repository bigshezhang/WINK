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
#include "imagedata.h"
#include "epd7in3f.h"
#include "LittleFS.h"
#include <AutoWifi.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

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

unsigned char dataArray[192000];

void UpdateEink(){
  Serial.print("更新数据中\n");
  HTTPClient http;
  http.begin("http://192.168.1.196:5001/api/photos");
  int httpCode = http.GET();
  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
          int len = http.getSize();
          Serial.print("数据长度为:");
          Serial.print(len);
          Serial.print('\n');

          // create buffer for read
          uint8_t buff[1280] = { 0 };
          // get tcp stream
          WiFiClient * stream = http.getStreamPtr();
          // read all data from server
          int numData = 0;
          String headString = "";
          while(http.connected() && (len > 0 || len == -1)) {
              // get available data size
              size_t size = stream->available();
              int c = 0;
              if(size) {
                  // read up to 1280 byte
                  c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                  String responseString((char*)buff, c);
                  responseString = headString + responseString;
                  String temp = ""; 
                  for (int i = 0; i < responseString.length(); i++) {
                    char cAti = responseString.charAt(i);
                    if (cAti == ',') { 
                      dataArray[numData] = temp.toInt();
                      Serial.print(dataArray[numData]);
                      temp = ""; // 清空临时字符串
                      numData++; // 数组索引加1
                    } else {
                      temp += cAti; // 将字符添加到临时字符串中
                    }
                  }
                  if (temp.length() > 0) { // 处理最后一个数字
                    headString = temp;
                  } else{
                    headString = "";
                  }
                  if(len > 0) {
                      len -= c;
                  }
                }
          }
      }
  }
  Serial.print("获取到了一个数据 \n");
  http.end();
}

void loop()
{
  UpdateEink();

  delay(1000);
}
