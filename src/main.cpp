#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecureBearSSL.h>
#include <ChatGPT.hpp>
#include "Secrets.h"

BearSSL::WiFiClientSecure client;
ChatGPT<BearSSL::WiFiClientSecure> chat_gpt(&client, "v1", openai_api_key);

inline bool message(String& result,
                    const String& model,
                    const String& role);

void setup() {
  Serial.begin(115200);
  Serial.println("Connecting to WiFi network: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting...");
    delay(1500);
  }
  Serial.println("Connected!");
}

void loop() {
  client.setInsecure();
  String result;
  if (!message(result, "gpt-3.5-turbo", "user")) {
    Serial.println("===ERROR===");
    Serial.println("Please try again.");
  }
}

inline bool message(String& result,
                    const String& model,
                    const String& role) {
  while (Serial.available()) {
    Serial.read();
  }
  String input;
  Serial.println("Enter your message: ");
  while (!input.endsWith("\n")) {
    input += Serial.readString();
  }
  input.trim();
  Serial.println("Synthesizing response...");
  if (chat_gpt.simple_message("gpt-3.5-turbo", "user", input, result)) {
    Serial.println(result);
    return true;
  } else {
    Serial.println("===ERROR===");
    Serial.println(result);
    return false;
  }          
} 
