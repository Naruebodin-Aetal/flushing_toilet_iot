#include <Arduino.h>
#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

Servo Myservo;
int Detectsensor = 17; // Define the pin for the detection sensor
void SensorDetect();
void sendSensorData(bool state);
unsigned long servoHoldUntil = 0;
const unsigned long SERVO_HOLD_MS = 5000;
bool lastSensorState = false; // Track previous sensor state for edge detection


// WIFI config
const char *ssid = "Wokwi-GUEST";
const char *password = "";
WiFiClient wifiClient;
// MQTT client config
const char *mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;
const char *mqttClientId = ""; // replace with your client ID
const char *mqttUser = ""; // replace with your MQTT username
const char *mqttPassword = ""; // replace with your MQTT password
const char *topic_pub = "@msg/lab_ict_kps/sensor_data/value"; // topic to publish
const char *data_pub = "@shadow/data/update"; // topic to publish
PubSubClient mqttClient(wifiClient);
// some variables buffer
String publishMessage;
// pin definition


void setup_wifi()
{
  // Start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("WiFi connected on ");
  Serial.print("IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print(" MAC address: ");
  Serial.println(WiFi.macAddress());
}

void reconnectMQTT()
{
  // Loop until reconnected
  char mqttinfo[80];
  char clientId[100] = "\0";
  snprintf(mqttinfo, 75, "Attempting MQTT connection at %s:%d (%s/%s)...", 
                          mqttServer, mqttPort, mqttUser, mqttPassword);
  while (!mqttClient.connected())
  {
    Serial.println(mqttinfo);
    
    if (mqttClient.connect(mqttClientId, mqttUser, mqttPassword))
    {
      Serial.println("---> MQTT Broker connected...");
      // subscribe here after connected
      // ในตัวอย่างของฝั่งส่งไม่มีการ subscribe topic ใด
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" try again in 5 seconds");      
      delay(5000);  // Recommended: Wait 5 seconds before retrying
    }
  }
}


void setup() {
  Serial.begin(115200);
  //setup_wifi();
  //mqttClient.setServer(mqttServer, mqttPort);
  Myservo.attach(26); 
  pinMode(Detectsensor, INPUT);
  delay(1000); // delay to let initialization ready  
}

void loop() {
  // if (!mqttClient.connected())
  // {
  //   reconnectMQTT();
  // } 
  // mqttClient.loop();
  
  SensorDetect(); // Call the function to check the sensor and move the servo accordingly
}


void SensorDetect() {
  bool currentSensorState = digitalRead(Detectsensor) == HIGH;

  // Edge detection: check if state changed
  if (currentSensorState != lastSensorState) {
    lastSensorState = currentSensorState;
    sendSensorData(currentSensorState); // Send MQTT only on state change
  }

  if (currentSensorState) {
    Myservo.write(90);
  } else {
    Myservo.write(0);
  }
}

void sendSensorData(bool state) {
  String status = state ? "DETECTED" : "CLEAR";
  publishMessage = "{\"sensor\":" + String(state ? 1 : 0) + ",\"status\":\"" + status + "\"}";
  
  mqttClient.publish(topic_pub, publishMessage.c_str());
  mqttClient.publish(data_pub, publishMessage.c_str());
  
  Serial.print("Sensor state changed to: ");
  Serial.print(status);
  Serial.print(" | Published: ");
  Serial.println(publishMessage);
}