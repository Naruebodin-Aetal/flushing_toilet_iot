#include <Arduino.h>
#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>

Servo Myservo;

// pin definition
int ServoPin = 26; // Define the pin for the servo motor
int Detectsensor = 17; // Define the pin for the detection sensor

void SensorDetect();
void sendSensorData(bool state);

// WIFI config
const char *ssid = "Redmi Note 14";
const char *password = "donpine000";
WiFiClient wifiClient;
// MQTT client config
const char *mqttServer = "mqtt.netpie.io";
const int mqttPort = 1883;
const char *mqttClientId = "8ad22b92-426b-4dc0-a575-2d36aeedee39";
const char *mqttUser = "m2EKkWEMDBuqkHW3E6WkhjTSUkjwQ8hi";
const char *mqttPassword = "8mvBLFxZRQAv5FK2pTwKE2aJ76Px9G6c";
const char *topic_pub = "@msg/lab_ict_kps/sensor_data/value"; // topic to publish
const char *topic_sub = "@msg/lab_ict_kps/flush/value"; // topic to subscribe
const char *data_pub = "@shadow/data/update"; // topic to publish
PubSubClient mqttClient(wifiClient);
// some variables buffer
String publishMessage;

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
  setup_wifi();
  mqttClient.setServer(mqttServer, mqttPort);
  Myservo.attach(ServoPin);
  pinMode(Detectsensor, INPUT);
  delay(1000); // delay to let initialization ready  
}

void loop() {
  if (!mqttClient.connected())
  {
    reconnectMQTT();
  } 
  mqttClient.loop();
  SensorDetect();
}

void SensorDetect() {
  bool currentSensorState = digitalRead(Detectsensor);

  if (currentSensorState) {
    //sendSensorData(currentSensorState);
    Myservo.write(90);
    delay(2000); // เวลาที่ใช้จับเซนเซอร์ตรวจจับ
    
  } else {



    Myservo.write(90); 
    delay(4000); // เวลาที่ใช้หมุนครั้งแรก

    Myservo.write(180);
    delay(4000); // เวลาที่ใช้หมุนครั้งที่สองก่อนหยุด

    Myservo.write(180);
    delay(4000); // เวลาที่ใช้หมุนครั้งที่สองก่อนหยุด
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