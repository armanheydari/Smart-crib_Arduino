#include "esp_camera.h"
#include <WiFi.h>


#define CAMERA_MODEL_AI_THINKER // Has PSRAM
#include "camera_pins.h"
#include "DHT.h"

#define DHTPIN 15 // Digital pin connected to thes DHT sensor
#define DHTTYPE DHT11   // DHT 11
#define pirPin 13


int soundPin=14;
boolean sound_status=0;
boolean motion_status=0;
boolean humidity_status=0;
boolean temprature_status=0;
float t, h, hic=0;
String header;
unsigned long currentTime = millis();
unsigned long previousTime = 0; 
const long timeoutTime = 2000;
WiFiServer server(90);

const char* ssid = "Arman";
const char* password = "82797982";
DHT dht(DHTPIN, DHTTYPE);

void startCameraServer();


void setup() {
  pinMode(soundPin, INPUT);
  pinMode(pirPin, INPUT);

  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();
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
  config.frame_size = FRAMESIZE_UXGA;
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  //config.pixel_format = PIXFORMAT_RGB565; // for face detection/recognition
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  
  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if(config.pixel_format == PIXFORMAT_JPEG){
    if(psramFound()){
      config.jpeg_quality = 10;
      config.fb_count = 2;
      config.grab_mode = CAMERA_GRAB_LATEST;
    } else {
      // Limit the frame size when PSRAM is not available
      config.frame_size = FRAMESIZE_SVGA;
      config.fb_location = CAMERA_FB_IN_DRAM;
    }
  } else {
    // Best option for face detection/recognition
    config.frame_size = FRAMESIZE_240X240;
    #if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
    #endif
  }

  #if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
  #endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t * s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // flip it back
    s->set_brightness(s, 1); // up the brightness just a bit
    s->set_saturation(s, -2); // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  if(config.pixel_format == PIXFORMAT_JPEG){
    s->set_framesize(s, FRAMESIZE_QVGA);
  }

  #if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
  #endif

  #if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
  #endif

  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("' to connect");
    
  xTaskCreatePinnedToCore(TaskWebServer90, "TaskWebServer90", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(SoundSensor, "SoundSensor", 10000, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(PIRSensor, "PIRSensor", 10000, NULL, 1, NULL, 1);
}

void loop() {
  DHTSensor();
  delay(10000);
}

void SoundSensor(void *pvParameters) {
  (void) pvParameters;
  while (true) {
    if (digitalRead(soundPin)==HIGH) {
      sound_status = 1;
    }
  }
}

void PIRSensor(void *pvParameters) {
    (void) pvParameters;
    while (true) {   
      if(digitalRead(pirPin) == HIGH) {
        delay(2000);
        if(digitalRead(pirPin) == HIGH) {
            motion_status = 1;
        }
      }
    }
}

void DHTSensor() {
      // dht.begin();  
    h = dht.readHumidity();
    // Read temperature as Celsius (the default)
    t = dht.readTemperature();
    
    // Check if any reads failed and exit early (to try again).
    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    // Compute heat index in Celsius (isFahreheit = false)
    hic = dht.computeHeatIndex(t, h, false);
}

void TaskWebServer90(void *pvParameters)  // This is a task.
{
  (void) pvParameters;
  server.begin();
  Serial.println("task WebServer90 started...");

  while (true) // A Task shall never return or exit.
  {
    WiFiClient client = server.available();   // Listen for incoming clients

    if (client) {                             // If a new client connects,
        currentTime = millis();
        previousTime = currentTime;
        String currentLine = "";                // make a String to hold incoming data from the client
        while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
        currentTime = millis();
        if (client.available()) {             // if there's bytes to read from the client,
            char c = client.read();             // read a byte, then
            Serial.write(c);                    // print it out the serial monitor
            header += c;
            if (c == '\n') {                    // if the byte is a newline character
            // if the current line is blank, you got two newline characters in a row.
            // that's the end of the client HTTP request, so send a response:
            if (currentLine.length() == 0) {
                client.println("HTTP/1.1 200 OK");
                client.println("Content-type:text/plain");
                client.println("Connection: close");
                client.println();
                
                
                if (header.indexOf("GET /mode") >= 0) {
                    client.print("Humidity: ");
                    client.print(h);
                    client.print("% \n");
                    client.print("Temprature: ");

                    client.print(t);
                    client.print(" Celcius \n");     
                    client.print("Heat index: ");
                    client.print(hic);
                    client.print(" Celcius \n");  
                    if (sound_status > 0){
                      client.println("sound detected.");
                    }                  
                    else {
                      client.println("no sound.");
                    }
                    if (motion_status > 0){
                      client.println("motion detected.");
                    }                  
                    else {
                      client.println("no motion.");
                    }
                    sound_status=0;
                    motion_status=0;
                    humidity_status=0;
                    temprature_status=0;
                }    
            
                break;
            } else { // if you got a newline, then clear currentLine
                currentLine = "";
            }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
            currentLine += c;      // add it to the end of the currentLine
            }
        }
        }
        // Clear the header variable
        header = "";
        // Close the connection
        client.stop();
    }

  }
}
