#include "esp_camera.h"
#include "Arduino.h"
#include "soc/soc.h"          // Disable brownour problems
#include "soc/rtc_cntl_reg.h" // Disable brownour problems
#include "driver/rtc_io.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <base64.h>
#include "base64.hpp"
#include "fd_forward.h"
// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Pin definition for CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

#define RXD2 16
#define TXD2 14

int pictureNumber = 0;
int counter = 0;
int limit = 0;

const char *ssid = "TP-Link_DECC";
const char *password = "9542584446";
const char *post_url = "http://smocklock2.herokuapp.com/api/recievefromESP32"; // Location where images are POSTED
const char *facial_rec_url = "http://face-rec751.herokuapp.com/doFacialRec";
bool internet_connected = false;
bool face_dec = false;
String payloader = "7";
//ESP32QRCodeReader reader(CAMERA_MODEL_WROVER_KIT);
//struct QRCodeData qrCodeData;
#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>

void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  //reader.setup();
  //reader.begin();

  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  //mySerial.begin(9600);
  //Serial.setDebugOutput(true);
  //Serial.println();
  if (init_wifi())
  { // Connected to WiFi
    internet_connected = true;
    Serial.println("Internet connected");
  }

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound())
  {
    config.frame_size = FRAMESIZE_CIF; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 10;
    config.fb_count = 2;
  }
  else
  {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Init Camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  //turn on low power mode(modem mode for now)
}

void callFacialRec()
{
  HTTPClient http;

  Serial.println("Calling Facial Rec.");

  http.begin(facial_rec_url); //HTTP

  http.addHeader("Content-Type", "application/json");

  int httpCode = http.POST("");

  // httpCode will be negative on error
  if (httpCode > 0)
  {
    // HTTP header has been send and Server response header has been handled
    String payload = http.getString();
    Serial.println(payload);
    payloader = payload;
  }
  else
  {
    Serial.printf("[HTTP1] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
   http.end(); 
}

void takePic()
{
  camera_fb_t *fb = NULL;
  
  // Take Picture with Camera
  fb = esp_camera_fb_get();
  if (!fb)
  {
    Serial.println("Camera capture failed");
    return;
  }

  //detect if face is in cam
  mtmn_config_t mtmn_config = {0};
  mtmn_config = mtmn_init_config();
  dl_matrix3du_t *image_matrix = dl_matrix3du_alloc(1, fb->width, fb->height, 3);
  fmt2rgb888(fb->buf, fb->len, fb->format, image_matrix->item);

  box_array_t *boxes = face_detect(image_matrix, &mtmn_config);

  if(boxes != NULL)
  {
    face_dec = true;
    HTTPClient http;

    Serial.print("[HTTP] begin...\n");
    // configure traged server and url

   http.begin(post_url); //HTTP

   Serial.print("[HTTP] POST...\n");
   // start connection and send HTTP header

   size_t size = fb->len;
   String buffer = base64::encode((uint8_t *) fb->buf, fb->len);

   String imgPayload = "{\"inputs\": [{ \"data\": {\"image\": {\"base64\": \"" + buffer + "\"}}}]}";

   //buffer = "";
   // Uncomment this if you want to show the payload
   //Serial.println(imgPayload);

   http.addHeader("Content-Type", "application/json");

   int httpCode = http.POST("{\"buffer\":\"" + buffer + "\"}"); // we simply put the whole image in the post body.

    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled

      // file found at server
      if (httpCode == HTTP_CODE_OK)
      {
       String payload = http.getString();
       //Serial.println(payload);
      }
    }
    else
    {
      Serial.printf("[HTTP] POST... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
   http.end(); 
   delay(2000);
   callFacialRec();
   }
   else
   {
      Serial.println("Camera failed to capture face");
   }

  esp_camera_fb_return(fb);
}

void loop()
{ 
  face_dec = false;
  while (Serial2.available())
  {
    while(face_dec == false)
    {
      limit++;
      takePic();
      if(face_dec == true)
      {
        Serial2.write("S");
      }
      else
      {
        Serial.println("Move Person");
        Serial2.write("L");
      }
      delay(8000);
    }
  }
  delay(8000);
}



bool init_wifi()
{
  int connAttempts = 0;
  Serial.println("\r\nConnecting to: " + String(ssid));
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    if (connAttempts > 10)
      return false;
    connAttempts++;
  }
  return true;
}

void setModemSleep() 
{
    WiFi.setSleep(true);
    if (!setCpuFrequencyMhz(40)){
        Serial2.println("Not valid frequency!");
    }
    // Use this if 40Mhz is not supported
    // setCpuFrequencyMhz(80);
}

void wakeModemSleep() 
{
    setCpuFrequencyMhz(240);
}
