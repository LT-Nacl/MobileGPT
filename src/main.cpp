#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
#include "Secrets.h"

BearSSL::WiFiClientSecure client;

void scan_and_display_networks();

inline bool connect(String ssid, String password, int timeout);

inline bool user_message(String& result, //this function actually takes input and displays output
                    const String& model,
                    const String& role);

inline bool chatgpt_req(BearSSL::WiFiClientSecure& client,
                        const String api_key,
                        const String& model,
                        const String& role,
                        const String& content,
                        String& result);

void process_json_response(const String& json_response);//needed to parse json for response

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
  client.setInsecure();//so we dont need a certificate for https reqs
  String result;
  if (!user_message(result, "gpt-3.5-turbo", "user")) {
    Serial.println("===ERROR===");
    Serial.println("Please try again.");
  }
}

void scan_and_display_networks() {
  int number_of_networks = WiFi.scanNetworks();
  for (int i = 0; i < number_of_networks; i++) {//we may want to sort by signal strength and only display the top amount at a later point in time
    Serial.print("Network name: ");
    Serial.println(WiFi.SSID(i));
    Serial.print("Signal strength: ");
    Serial.println(WiFi.RSSI(i));
    Serial.println("-----------------------");
    delay(600);//only reason for delay is to make it easier to read
  }
}

inline bool connect(String ssid, String password, int timeout) {
  WiFi.begin(ssid, password);
  int attempts = 0;//connection attempts
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
    process_json_response(result);
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
  String response_str;
  String api_version = "v1";
  String host = "api.openai.com";
  String auth_header = "Authorization: Bearer " + api_key;
  String post_body = "{\"model\": \"" + model + "\", \"messages\": [{\"role\": \"" + role + "\", \"content\": \"" + content + "\"}]}";
  String payload = "POST /" + api_version + "/chat/completions HTTP/1.1\r\n" + auth_header + "\r\n" + "Host: " + host + "\r\n" + "Cache-control: no-cache\r\n" + "User-Agent: ESP32 ChatGPT\r\n" + "Content-Type: application/json\r\n" + "Content-Length: " + post_body.length() + "\r\n" + "Connection: close\r\n" + "\r\n" + post_body + "\r\n";
  if (client.connect(host, 443)) {
    client.print(payload);
    while (client.connected()) {
      if (client.available()) {
        response_str += client.readString();
      }
    }
    client.stop(); 
    int response_code = 0;
    if (response_str.indexOf(" ") != -1) {
      response_code = response_str.substring(response_str.indexOf(" ") + 1, response_str.indexOf(" ") + 4).toInt();
    }

    if (response_code != 200) {
      result = response_str;
      return false;
    } else {

      int start = response_str.indexOf("{");
      int end = response_str.lastIndexOf("}");
      String json_body = response_str.substring(start, end + 1);

      if (json_body.length() > 0) {
        result = json_body;
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

void process_json_response(const String& json_response) {
  DynamicJsonDocument doc(json_response.length() + 200);

  DeserializationError error = deserializeJson(doc, json_response.c_str());

  if (error) {//we may or may not want all of this error handling
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  if (!doc.containsKey("choices")) {
    Serial.println("JSON response does not contain 'choices' array.");
    return;
  }

  JsonArray choices_array = doc["choices"].as<JsonArray>();

  if (choices_array.size() == 0) {
    Serial.println("JSON response 'choices' array is empty.");
    return;
  }

  JsonObject first_choice = choices_array[0].as<JsonObject>();

  if (!first_choice.containsKey("message")) {
    Serial.println("JSON response element does not contain 'message' object.");
    return;
  }

  JsonObject message_object = first_choice["message"].as<JsonObject>();

  if (!message_object.containsKey("content")) {
    Serial.println("JSON response 'message' object does not contain 'content' field.");
    return;
  }

  const char* message_content = message_object["content"].as<const char*>();

  Serial.println(message_content);
}