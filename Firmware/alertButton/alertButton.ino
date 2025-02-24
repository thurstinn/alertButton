/*
  LSteen
  Complete project details at https://github.com/thurstinn/alertButton
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <esp_sleep.h>
#include <driver/rtc_io.h>
#include <Pushsafer.h>

#define alertBtn 21
#define led 36
#define batV 10
#define uS_TO_S_FACTOR 1000000ULL
#define TIME_TO_SLEEP 86400  // 24 hours in seconds
#define WIFI_TIMEOUT 5000
#define PushsaferKey "JCJtpSI4z3gLTH7Te254"

int adcRead;
float batVoltage;
unsigned long buttonPressStart = 0;
unsigned long isAwakeStart = 0;
const float lowBat = 3.5;
const unsigned long holdTime = 5000;
const unsigned long awakeTime = 15000;
static int lastButtonState = LOW;
static int buttonState = LOW;
static bool sendingNotification = false;
static bool notificationSent = false;
esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();
RTC_DATA_ATTR bool isAwake = false;

const char* ssid = "Bet5G";
const char* password = "betjo5775";

const char* ssid2 = "Leighty";
const char* password2 = "JC2ax6w@Q52P";

const char* apiToken = "ahugybyh2xq23vq158yhowgwz1ozyu";
const char* userToken = "unt1r4nn7iys1n47u4uekr2qhq56np";

const char* pushoverApiEndpoint = "https://api.pushover.net/1/messages.json";
const char *PUSHOVER_ROOT_CA = "-----BEGIN CERTIFICATE-----\n"
                  "MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh\n"
                  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
                  "d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
                  "MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT\n"
                  "MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j\n"
                  "b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG\n"
                  "9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI\n"
                  "2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx\n"
                  "1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ\n"
                  "q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz\n"
                  "tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ\n"
                  "vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP\n"
                  "BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV\n"
                  "5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY\n"
                  "1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4\n"
                  "NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG\n"
                  "Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91\n"
                  "8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe\n"
                  "pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl\n"
                  "MrY=\n"
                  "-----END CERTIFICATE-----\n";

WiFiClientSecure client;
WiFiClient clientz;
Pushsafer pushsafer(PushsaferKey, clientz);


void setup() {
  Serial.begin(115200);
  delay(1000); 
  pinMode(alertBtn, INPUT);
  pinMode(led, OUTPUT);
  digitalWrite(led, HIGH);

  isAwakeStart = millis();
  isAwake = true;

  esp_sleep_enable_ext0_wakeup(GPIO_NUM_21, 1);
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  print_wakeup_reason(wakeup_reason);

  Serial.println("Trying WiFi #1...");
  connectToWiFi(ssid, password);

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi #1 failed! Trying WiFi #2...");
    connectToWiFi(ssid2, password2);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("✅ Connected to WiFi!");
  } else {
    Serial.println("❌ Failed to connect to both WiFi networks.");
  }

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) { 
    checkBattery(); 
    delay(100); 
  }
  batVoltage = readBatVoltage();
  Serial.print("Bat Voltage: ");
  Serial.print(batVoltage);
}

void connectToWiFi(const char* ssid, const char* password) {
  WiFi.begin(ssid, password);
  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < WIFI_TIMEOUT) {
    delay(300);
    Serial.println("Connecting to WiFi...");
  }
}

float readBatVoltage() {
  analogReadResolution(12);
  adcRead = analogRead(batV);
  batVoltage = (float)adcRead * (3.3 / 5000) * 2;
  return batVoltage;
}

int readPushsaferResponse() {
  Serial.println("Reading HTTP response...");

  while (clientz.available() == 0) {
    delay(100);  // Wait for response
  }
    
  String response;
  while (clientz.available()) {
    response += clientz.readString();  // Read full response
  }

  Serial.println("Full HTTP Response:");
  Serial.println(response);

    // ** Extract JSON Payload from Response **
  int jsonStart = response.indexOf('{');  // Find start of JSON
  if (jsonStart == -1) {
    Serial.println("JSON not found in response!");
    return -1;  // Return error code
  }

  String jsonString = response.substring(jsonStart);  // Extract JSON part

    // ** Parse JSON Response **
  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, jsonString);

  if (error) {
    Serial.print("JSON Parse Error: ");
    Serial.println(error.f_str());
    return -1;  // Return error code
  }

  int status = doc["status"];  // Extract "status" field

  Serial.print("Extracted Status Code: ");
  Serial.println(status);

  return status;  // Return the actual status code
}

void checkBattery() {
  batVoltage = readBatVoltage();
  Serial.print("Bat Voltage: ");
  Serial.print(batVoltage);
  if (batVoltage <= lowBat) {
    Serial.println(", Battery low - Sending notification...");
    sendNotification(("Battery Low! Please recharge. VBat = " + String(batVoltage, 2)).c_str(), "vibrate", "0", "3600", "10800");
  } else {
    sendNotification(("alertButton VBat = " + String(batVoltage, 2)).c_str(), "vibrate", "0", "3600", "10800");
  }
}

void flashLed() {
  for (int i = 0; i < 10; i++) {
    digitalWrite(led, LOW);
    delay(100);
    digitalWrite(led, HIGH);
    delay(100);
  }
}

void print_wakeup_reason(esp_sleep_wakeup_cause_t reason) {
  switch (reason) {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Woken by Button.");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Woken by Timer.");
      break;
    default:
      Serial.println("Woken by Reset.");
      break;
  }
}

void handleButtonPress() {
  if (digitalRead(alertBtn) == HIGH) { 
    if (buttonPressStart == 0) {
      buttonPressStart = millis();
      Serial.println("Button pressed - Starting timer...");
      digitalWrite(led, LOW);
      sendingNotification = true;
    }
    if (millis() - buttonPressStart >= holdTime && !notificationSent) {
      Serial.println("Button held for 5 seconds - Sending notification...");
      sendNotification("!!!!ALERT: Button Pressed, Call Mom!!!!", "updown", "2", "30", "10800");
      notificationSent = true;
      digitalWrite(led, HIGH);
    }
  } else {
    if (buttonPressStart > 0) {
      Serial.println("Button released - Resetting state...");
      buttonPressStart = 0;
      sendingNotification = false;
      notificationSent = false;
      digitalWrite(led, HIGH);
    }
  }
}

void sendNotification(const char* message, const char* sound, const char* priority, const char* retry, const char* expire) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected. Attempting to reconnect...");
    connectToWiFi(ssid, password);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi #1 failed! Trying WiFi #2...");
    connectToWiFi(ssid2, password2);
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✅ Wifi reconnected!");
    } else {
      Serial.println("❌ Failed to reconnect to WiFi");
      return; // Exit if WiFi cannot reconnect
    }
  }

  if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) { 
    checkBattery(); 
    delay(100); 
  } 

  
  client.setCACert(PUSHOVER_ROOT_CA);
  
  HTTPClient https;
  https.begin(client, pushoverApiEndpoint);
  https.addHeader("Content-Type", "application/json");

  StaticJsonDocument<512> notification;
  notification["token"] = apiToken;
  notification["user"] = userToken;
  notification["message"] = message;
  notification["title"] = "alertButton";
  notification["sound"] = sound;
  notification["priority"] = priority;
  notification["retry"] = retry;
  notification["expire"] = expire;

  String jsonStringNotification;
  serializeJson(notification, jsonStringNotification);

  int httpResponseCode = https.POST(jsonStringNotification); //Send notification
  
  if (httpResponseCode > 0) { //Verify notification    
    flashLed();         
    Serial.printf("✅ Pushover Notification sent Successfully!: %s\n", message);
  } else {
    Serial.printf("❌ Failed to send Pushover notification. Code: %d\n", httpResponseCode);
    Serial.printf("Trying backup Pushsafer service....");
    //Setup alternate service, Pushsafer
    pushsafer.debug = true;
    struct PushSaferInput input;
    input.message = "!!!!ALERT: Button Pressed, Call Mom!!!!";
    input.title = "alertButton Alt";
    input.sound = "27";
    input.vibration = "1";
    input.icon = "5";
    input.iconcolor = "#FF0000";
    input.priority = "2";
    input.device = "a";
    input.url = "";
    input.urlTitle = "";
    input.picture = "";
    input.picture2 = "";
    input.picture3 = "";
    input.time2live = "";
    input.retry = "60";
    input.expire = "10800";
    input.confirm = "30";
    input.answer = "";
    input.answeroptions = "";
    input.answerforce = "1";
    pushsafer.sendEvent(input);

    int statusCode = readPushsaferResponse();

    if (statusCode > 0) {
      flashLed();
      Serial.println("✅ Pushsafer Notification Sent Successfully!");
    } else {
      Serial.println("❌ Failed to Send Pushsafer Notification!");
    }
  }
  https.end();
  delay(2000);
  sendingNotification = false;
}

void loop() {
  if (isAwake) {
    handleButtonPress();
    if (!sendingNotification) {  
      if (millis() - isAwakeStart >= awakeTime) {
        Serial.println("Timer elapsed, entering deep sleep...");
        isAwake = false;
        digitalWrite(led, HIGH);
        esp_deep_sleep_start();
      }
    }
  } else {
    // If somehow awake without the proper flags, reset state and go back to sleep
    Serial.println("Unexpected wake-up. Going back to sleep...");
    esp_deep_sleep_start();
  }
}




