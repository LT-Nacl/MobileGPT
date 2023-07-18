#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ChatGPT.hpp>
#include "Secrets.h"

BearSSL::WiFiClientSecure client;
ChatGPT<BearSSL::WiFiClientSecure> chat_gpt(&client, "v1", openai_api_key);

void processJsonResponse(const String& jsonResponse);

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(500);
  }
  Serial.println("Connected!");

  client.setInsecure();

  String result;
  bool call = chat_gpt.simple_message("gpt-3.5-turbo", "user", "Hello", result);
  //if (chat_gpt.simple_message("gpt-3.5-turbo-0301", "user", "Planning a 3-day trip to San Diego", result)) {
  //Serial.println("===OK===");
  Serial.println(result);
  /*} else {
    Serial.println("===ERROR===");
    Serial.println(result);
  }*/
}

void loop() {}
