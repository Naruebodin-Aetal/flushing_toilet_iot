#include <Arduino.h>
#include <Servo.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <time.h>

Servo Myservo;

// pin definition
int ServoPin = 26; // Define the pin for the servo motor
int Detectsensor = 17; // Define the pin for the detection sensor

void SensorDetect();
void sendSensorData(bool state);

// WIFI config
const char *ssid = "Redmi Note 14";
const char *password = "donpine000";
// const char *ssid = "iPhone (7)";
// const char *password = "245566677)";
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
bool isGettingCommand;

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
      mqttClient.subscribe(topic_sub);
      
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message, publishMessage;
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");

  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);

  if (message.indexOf("\"isFlushing\":1")) {
    isGettingCommand = true;
  } 
  
}

void sendToFirebase() {
  HTTPClient http;
  String url = "https://toliet-project-default-rtdb.asia-southeast1.firebasedatabase.app/sensor_data.json"; // URL ของ Firebase Realtime Database (ปรับให้ตรงกับโครงสร้างของคุณ)

  // สร้าง timestamp
  time_t now = time(nullptr);
  struct tm* timeinfo = localtime(&now);
  char timestamp[25];
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", timeinfo);

  // สร้าง entry ID แบบสุ่มหรือใช้ millis()
  String entryId = "entry" + String(millis());

  // สร้าง payload JSON
  String payload = "{ \"" + entryId + "\": {";
  payload += "\"timestamp\": \"" + String(timestamp) + "\"";
  payload += "} }";

  http.begin(url);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.PATCH(payload); // ใช้ PATCH เพื่อ merge entry ใหม่

  if (httpResponseCode > 0) {
    Serial.println("Firebase response: " + http.getString());
  } else {
    Serial.println("Error sending to Firebase. Code: " + String(httpResponseCode));
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  setup_wifi();
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(mqttCallback);
  // time setup
  configTime(7 * 3600, 0, "pool.ntp.org"); // GMT+7 สำหรับประเทศไทย
  Serial.println("Waiting for NTP time sync...");
  while (time(nullptr) < 100000) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nTime synced!");
  Myservo.attach(ServoPin);
  pinMode(Detectsensor, INPUT);
  isGettingCommand = false;
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
  bool currentSensorState ;
  if(isGettingCommand){
    isGettingCommand = false; // i dont know why
    currentSensorState = false;
  }else{
    currentSensorState = digitalRead(Detectsensor); 
  }
  sendSensorData(currentSensorState);
  if (currentSensorState) {
   
    Myservo.write(90);
    delay(2000); // เวลาที่ใช้จับเซนเซอร์ตรวจจับ
    
  } else {
    sendToFirebase();
    // Myservo.write(90); 
    // delay(4000); // เวลาที่ใช้หมุนครั้งแรก

    Myservo.write(180);
    delay(4000); // เวลาที่ใช้หมุนครั้งที่สองก่อนหยุด

    Myservo.write(0);
    delay(4000); // เวลาที่ใช้หมุนครั้งที่สองก่อนหยุด

    
  }
}

void sendSensorData(bool state) {
  publishMessage = "{\"data\":";
  publishMessage += "{\"isFlushing\":" + String(state ? 0 : 1);
  publishMessage += "}";
  publishMessage += "}";
  mqttClient.publish(topic_pub, publishMessage.c_str());
  mqttClient.publish(data_pub, publishMessage.c_str());
  
  Serial.print("Sensor state changed to: ");
  Serial.print(" | Published: ");
  Serial.println(publishMessage);
}