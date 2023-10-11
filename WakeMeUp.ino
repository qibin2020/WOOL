/**
   WakeMeUp.ino

   Based on example: https://github.com/espressif/arduino-esp32/blob/master/libraries/HTTPClient/examples/BasicHttpsClient/BasicHttpsClient.ino

    This should work with ESP32C3 superMini board. Play as you like. 

    L.
*/

#include <Arduino.h>

#include <WiFi.h>
#include <WiFiMulti.h>

#include <HTTPClient.h>

#include <WiFiClientSecure.h>

///////////////////////// cert for https connection; replace by yourselves
#include "your_cute_server.h"
//#include "github.h" // e.g. if you host your server with github

//////////////////////// deep sleep module
#define uS_TO_S_FACTOR 1000000ULL  /* Conversion factor for micro seconds to seconds */
#define TIME_TO_SLEEP  600        /* Time ESP32 will go to sleep (in seconds) */

RTC_DATA_ATTR int bootCount = 0;

void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch(wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }
}


///////////////////////////// ping module
#include <ESPping.h>
bool testStatus(){ // test whether PC really wake up
 delay(1000);
 const IPAddress remote_ip(192,168,1,XX); // IP address of the target PC. Better set with DHCP bonding.
 return Ping.ping(remote_ip) > 0 ;
}
///////////////////////////// WOL payload
#include <WiFiUdp.h>
WiFiUDP UDP;
#include <WakeOnLan.h>
WakeOnLan WOL(UDP); // Pass WiFiUDP class

void prepareWOL(){
  WOL.setRepeat(3, 100); // Repeat the packet 10 times with 100ms delay between
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask());
  // const IPAddress broadcast_ip(192,168,1,255); // manually set the broadcast address
  // WOL.setBroadcastAddress(broadcast_ip);
}

void sendWOL(){
  const char *MACAddress = "XX:XX:XX:XX:XX:XX"; // The MAC of LAN adapter of PC. 
  WOL.sendMagicPacket(MACAddress);
  Serial.println("WOL emmit!");
  Serial.println("Waiting 1min...");
  delay(1000*60);
}

/////////////////////// Test request
bool testRequest(){
  bool ret=false;
  WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client -> setCACert(rootCACertificate);

    {
      // Add a scoping block for HTTPClient https to make sure it is destroyed before WiFiClientSecure *client is 
      HTTPClient https;
  
      Serial.print("[HTTPS] begin...\n");
      if (https.begin(*client, "https://your_cute_server/todo.html")) {  // address to monitor. e.g. here if return "On" means to wake up
        // of course you can use github but the latency would be high.
        Serial.print("[HTTPS] GET...\n");
        // start connection and send HTTP header
        int httpCode = https.GET();
  
        // httpCode will be negative on error
        if (httpCode > 0) {
          // HTTP header has been send and Server response header has been handled
          Serial.printf("[HTTPS] GET... code: %d\n", httpCode);
  
          // file found at server
          if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
            String payload = https.getString();
            Serial.println(payload);
            if (payload.indexOf("On") >= 0){
                Serial.println("REQUEST PC On!");
                ret=true;
            }
          }
        } else {
          Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
        }
  
        https.end();
      } else {
        Serial.printf("[HTTPS] Unable to connect\n");
      }

      // End extra scoping block
    }
  
    delete client;
  } else {
    Serial.println("Unable to create client");
  }          
   return ret;
}

// Not sure if WiFiClientSecure checks the validity date of the certificate. 
// Setting clock just to be sure...
void setClock() {
  configTime(0, 0, "pool.ntp.org");

  Serial.print(F("Waiting for NTP time sync: "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }

  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current time: "));
  Serial.print(asctime(&timeinfo));
}


WiFiMulti WiFiMulti;

void setup() {
  pinMode(8, OUTPUT);      // set the LED pin mode
  digitalWrite(8, LOW);  // for fun

  Serial.begin(115200);
  delay(1000);
  
  ++bootCount;
  Serial.println("Boot number: " + String(bootCount));
  if(bootCount>9999) bootCount=0;
  print_wakeup_reason();

  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  Serial.println("Setup ESP32 to sleep for every " + String(TIME_TO_SLEEP) + " Seconds");
  
  // Serial.setDebugOutput(true);

  Serial.println();
  Serial.println();
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP("YOUR_WIFI_NAME", "YOUR_WIFI_PASSWORD");

  // wait for WiFi connection
  Serial.print("Waiting for WiFi to connect...");
  while ((WiFiMulti.run() != WL_CONNECTED)) {
    Serial.print(".");
  }
  Serial.println(" connected");

  setClock();  

  prepareWOL();
}

void loop() {

  if(testStatus()){
    Serial.println("Target already On. Deep Sleep 10 min");
    Serial.println("Going to sleep now");
    Serial.flush(); 
    esp_deep_sleep_start();
    Serial.println("This will never be printed");
//    delay(1000*60*10);
  }else{
    if(testRequest()){
      for(int i=0;i<5;i++){
        sendWOL();
        if(testStatus()) {
          Serial.println("WOL DONE!");
          break;
        }
      }
      Serial.println("WOL FAILED!");
    }else{
      Serial.println("No requst. Wait 1min...");
      delay(1000*60*1);
    }
  }
  
//  Serial.println();
//  Serial.println("Waiting 10s before the next round...");
//  delay(10000);
}
