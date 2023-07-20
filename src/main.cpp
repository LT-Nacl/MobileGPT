#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "Secrets.h"

BearSSL::WiFiClientSecure client;

void scan_and_display_networks();

inline bool connect(String ssid, String password, int timeout);

inline bool user_message(String& result,
                    const String& model,
                    const String& role);

inline bool chatgpt_req(BearSSL::WiFiClientSecure& client,
                        const String api_key,
                        const String& model,
                        const String& role,
                        const String& content,
                        String& result);

void processJsonResponse(const String& jsonResponse);

void setup() {
  Serial.begin(115200);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Enter Wifi name or \"scan\" to scan for networks: ");
    String input;
    while (!input.endsWith("\n")) {
      input += Serial.readString();
    }
    input.trim();
    if (input == "scan") {
      scan_and_display_networks();
      continue;
    } else {
      Serial.println("Enter Wifi password:");
      String password;
      while (!password.endsWith("\n")) {
        password += Serial.readString();
      }
      password.trim();
      if (connect(input, password, 10)) {
        break;
      } else {
        Serial.println("Connection failed. Please try again.");
      }
    }
    delay(500);
  }
  Serial.println("Connected.");
}
void loop() {
  client.setInsecure();
  String result;
  if (!user_message(result, "gpt-3.5-turbo", "user")) {
    Serial.println("===ERROR===");
    Serial.println("Please try again.");
  }
}

void scan_and_display_networks() {
  int numberOfNetworks = WiFi.scanNetworks();
  for (int i = 0; i < numberOfNetworks; i++) {
    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
    Serial.println("-----------------------");
    delay(1000);
  }
}

inline bool connect(String ssid, String password, int timeout) {
  WiFi.begin(ssid, password);
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED and  attempts < timeout) {
    Serial.println("Connecting...");
    delay(1500);
    attempts++;
  }
  return WiFi.status() == WL_CONNECTED;
}

inline bool user_message(String& result,
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
  if (chatgpt_req(client, String(openai_api_key), "gpt-3.5-turbo", "user", input, result)) {
    processJsonResponse(result);
    return true;
  } else {
    Serial.println("===ERROR===");
    Serial.println(result);
    return false;
  }          
} 

inline bool chatgpt_req(BearSSL::WiFiClientSecure& client,
                        const String api_key,
                        const String& model,
                        const String& role,
                        const String& content,
                        String& result) {
  String responseStr;
  String api_version = "v1";
  String host = "api.openai.com";
  String auth_header = "Authorization: Bearer " + api_key;
  String post_body = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"" + role + "\", \"content\": \"" + content + "\"}]}";
  String payload = "POST /" + api_version + "/chat/completions HTTP/1.1\r\n" + auth_header + "\r\n" + "Host: " + host + "\r\n" + "Cache-control: no-cache\r\n" + "User-Agent: ESP32 ChatGPT\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + post_body.length() + "\r\n" + "Connection: close\r\n" + "\r\n" + post_body + "\r\n";
  if (client.connect(host, 443)) {
    client.print(payload);
    while (client.connected()) {
      if (client.available()) {
        responseStr += client.readString();
      }
    }
    client.stop(); 
    int responseCode = 0;
    if (responseStr.indexOf(" ") != -1) {
      responseCode = responseStr.substring(responseStr.indexOf(" ") + 1, responseStr.indexOf(" ") + 4).toInt();
    }

    if (responseCode != 200) {
      result = responseStr;
      return false;
    } else {

      int start = responseStr.indexOf("{");
      int end = responseStr.lastIndexOf("}");
      String jsonBody = responseStr.substring(start, end + 1);

      if (jsonBody.length() > 0) {
        result = jsonBody;
        return true;
      }

      result = "[ERR] Couldn't read data";
      return false;
    }
    return true;
  } else {
    result = "[ERR] Connection failed!";
    return false;
  }
}

void processJsonResponse(const String& jsonResponse) {
  DynamicJsonDocument doc(jsonResponse.length() + 200);

  DeserializationError error = deserializeJson(doc, jsonResponse.c_str());

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  if (!doc.containsKey("choices")) {
    Serial.println("JSON response does not contain 'choices' array.");
    return;
  }

  JsonArray choicesArray = doc["choices"].as<JsonArray>();

  if (choicesArray.size() == 0) {
    Serial.println("JSON response 'choices' array is empty.");
    return;
  }

  JsonObject firstChoice = choicesArray[0].as<JsonObject>();

  if (!firstChoice.containsKey("message")) {
    Serial.println("JSON response element does not contain 'message' object.");
    return;
  }

  JsonObject messageObject = firstChoice["message"].as<JsonObject>();

  if (!messageObject.containsKey("content")) {
    Serial.println("JSON response 'message' object does not contain 'content' field.");
    return;
  }

  const char* messageContent = messageObject["content"].as<const char*>();

  Serial.println(messageContent);
}