/*
 * test a combination of JSON, MQTT over Arduino-Ethernet
 * simple sensor: button
 * simple actuator: LED
 */

#include <Ethernet.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

// i/o pin map
const int led0 = 5;
const int button0 = 2;
// analog sensor on A0

// Ethernet: define MAC address of Ethernet shield
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {0x90, 0xA2, 0xDA, 0x00, 0x4D, 0x61};
EthernetClient ethClient;

// PubSub (MQTT)
const char* mqttServer = "mqttbroker";
// alternative: IPAddress mqttServer(172, 16, 0, 2);
const int mqttPort = 1883;

PubSubClient client(ethClient);

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
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for (;;)
      ;
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print(".");
  }
  Serial.println();
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
  pinMode(led0, OUTPUT);
  pinMode(button0, INPUT);
  analogReference(INTERNAL); //1.1V
  Serial.begin(9600);

  networkSetup();
  nodeID = String(mac[4], HEX) + String(mac[5], HEX);
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
