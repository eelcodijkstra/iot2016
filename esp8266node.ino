/*
 * test a combination of JSON, MQTT over ESP8266-WiFi
 * simple sensor: button
 * simple actuator: LED
 */

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// i/o pin map
const int led0 = D5;
const int button0 = D4;

// WiFi
const char* ssid     = "networkname";
const char* password = "password";
unsigned char mac[6];
WiFiClient espClient;

// PubSub (MQTT)
const char* mqttServer = "mqttbroker.com";
// alternative: IPAddress mqttServer(172, 16, 0, 2);
const int mqttPort = 1883;

PubSubClient client(espClient);

String nodeID;
String sensorTopic;
String actuatorTopic;

long sensor1Timer = 0;
long sensor1Period = 50000; // in millisecs

// JSON

void sensor0Publish() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String msg;
  root["id"] = nodeID;
  root["sensor0"] = digitalRead(button0);
  root["localtime"] = millis();
  root.printTo(msg);
  Serial.println(msg);
  client.publish(sensorTopic.c_str(), msg.c_str());
}

void sensor1Publish() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String msg;
  root["id"] = nodeID;
  root["sensor1"] = analogRead(A0);
  root["localtime"] = millis();
  root.printTo(msg);
  Serial.println(msg);
  client.publish(sensorTopic.c_str(), msg.c_str());
}

void networkSetup() {
  digitalWrite(BUILTIN_LED, LOW); // active low: LED ON
  delay(100);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(ssid);

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
 
  Serial.println();
  Serial.print("WiFi connected, IP address: ");  
  Serial.println(WiFi.localIP());
  WiFi.macAddress(mac);
  Serial.print("MAC address: ");
  for (int i = 0; i < 6; i++) {
    Serial.print(mac[i], HEX);
  }
  Serial.println();

  digitalWrite(BUILTIN_LED, HIGH); // LED off
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  if (strcmp(topic, actuatorTopic.c_str())==0) {
    Serial.println("actuator message received");
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject((char*) payload);
    if (root.success()) {
      if (root.containsKey("led0")) {
        digitalWrite(led0, root["led0"]);
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientID = "IoTClient-" + nodeID;
    if (client.connect(clientID.c_str())) {
      Serial.println("connected");

      client.subscribe(actuatorTopic.c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  pinMode(led0, OUTPUT);
  pinMode(button0, INPUT);
  Serial.begin(115200);
  
  networkSetup();
  nodeID = String(mac[4] * 256 + mac[5], HEX);
  // MQTT init:
  sensorTopic = "node/" + nodeID + "/sensors";
  actuatorTopic = "node/" + nodeID + "/actuators";
  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (digitalRead(button0)) {
    sensor0Publish();
    delay(200); // limit button repetition rate
  }

  if (millis() >= sensor1Timer) {
    sensor1Publish();
    sensor1Timer = sensor1Timer + sensor1Period;
  }
}
