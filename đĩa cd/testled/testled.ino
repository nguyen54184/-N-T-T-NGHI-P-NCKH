#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <string.h>

const int DHT_PIN = 18;
#define DHTTYPE DHT11

//led
const int ledPin = 23;
const int ledPin2 = 19;
const int ledPin3 = 13;
const int ledPin4 = 12;
// //led RGB
// const int pinRed = 12;
// const int pinGreen = 11;
// const int pinBlue = 10;
//fan 
const int RELAY = 5;

char result_temp[10];  // Buffer big enough for 7-character float
char result_hum[10];   // Buffer big enough for 7-character float

const char *payloadDHT;
// const char *prev_data = "";
unsigned long t_tick = 0;

//fire
const int cambien = 4;  //Chân cảm biến nối chân số 5 Arduino
int giatri;

const int coi = 17; // coi



//rain
const int rainSensor = 34;  // Chân tín hiệu cảm biến mưa ở chân digital 6 (arduino)

//wifi
const char* ssid = "RedmiNote12";
const char* password = "12345678";
WiFiClient espClient;
DHT dht(DHT_PIN, DHTTYPE);
//MQTT
PubSubClient client(espClient);

// MQTT Broker
const char *mqtt_broker = "broker.emqx.io";
const int mqtt_port = 1883;

// topic
const char *DhtTopic = "ESP32/DHT11/Data";
const char *LighTopic = "ESP32/LIGHT/Data";
const char *Ligh2Topic = "ESP32/LIGHT2/Data";
const char *Ligh3Topic = "ESP32/LIGHT3/Data";
const char *Ligh4Topic = "ESP32/LIGHT4/Data";
const char *RainTopic = "ESP32/RAIN/data";
const char *FireTopic = "ESP32/FIRE/data";
const char *RelayTopic = "ESP32/RELAY/data";
const char *RgbTopic = "ESP32/LIGHT_RGB/Data";
const char *RGBStatusTopic = "ESP32/LIGHT_RGB_STATUS/Data";

//auth
// const char *mqtt_username = "iot-class";
// const char *mqtt_password = "iotclass123";

int red, green, blue;

void setup() {
  Serial.begin(115200);
  //LED
  pinMode(ledPin, OUTPUT);
  pinMode(ledPin2, OUTPUT);
  pinMode(ledPin3, OUTPUT);
  pinMode(ledPin4, OUTPUT);
  pinMode(RELAY, OUTPUT);
  pinMode(coi, OUTPUT);
  digitalWrite(coi,LOW);
  pinMode(rainSensor,INPUT);
  //RGB
  // pinMode(pinRed, OUTPUT);
  // pinMode(pinGreen, OUTPUT);
  // pinMode(pinBlue, OUTPUT);
  //
  dht.begin();
  connect_wifi();
  connect_subscribe_mqtt();
  client.setCallback(callback);
  publish_mqtt();
}

void connect_wifi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {  //Check for the connection
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connect_subscribe_mqtt() {
  client.setServer(mqtt_broker, mqtt_port);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str())) {
      Serial.println("Mqtt broker connected");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(1000);
    }
  }
  client.subscribe(LighTopic);
  client.subscribe(Ligh2Topic);
  client.subscribe(Ligh3Topic);
  client.subscribe(Ligh4Topic);
  client.subscribe(RgbTopic);
  client.subscribe(RelayTopic);
  client.subscribe(RGBStatusTopic);
}

void publish_mqtt() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  delay(1000);
  dtostrf(temperature, 6, 2, result_temp);  // Leave room for too large numbers!
  dtostrf(humidity, 6, 2, result_hum);      // Leave room for too large numbers!

  StaticJsonDocument<200> docDHT;
  // Add values in the document
  docDHT["Temperature"] = result_temp;
  docDHT["Humidity"] = result_hum;

  String dataSensor;
  serializeJson(docDHT, dataSensor);

  payloadDHT = dataSensor.c_str();
  Serial.println("Publishing: ");
  client.publish(DhtTopic, payloadDHT);
  Serial.println(payloadDHT);
}

void publish_mqtt_Fire() {
    giatri = digitalRead(cambien);  //Đọc giá trị digital từ cảm biến và gán vào biến giatri
    if (giatri == 0) {
        client.publish(FireTopic, "fire");
         Serial.print("CHay ");
         digitalWrite(coi , HIGH); 
         // Here
    } else if (giatri < 0) {
        client.publish(FireTopic, "not fire");
         Serial.print("khong ");
         digitalWrite(coi,LOW); // And here
    }
}

void publish_mqtt_Rain() {
 int value = digitalRead(rainSensor);  //Đọc tín hiệu cảm biến mưa
  if (value == HIGH) {  
    client.publish(RainTopic, "not Rain");
    Serial.println("không mưa");
  } else {
    client.publish(RainTopic, "Rain");  
  }
  delay(2000);
}

void control_device(String input_topic, String check_topic, String status, int pin) {
  if (String(input_topic) == check_topic) {
    // Serial.print("Changing output to ");
    if (String(status) == "true") {
      Serial.println("on");
      digitalWrite(pin, HIGH);
    } else if (status == "false") {
      // Serial.println("off");
      digitalWrite(pin, LOW);
    }
  }
}

void callback(char *topic, byte *payload, unsigned int length) {

  String statusLed, statusLed2,statusLed3,statusLed4, statusFan,statusRGB;

  for (int i = 0; i < length; i++) {
    statusLed += (char)payload[i];
    statusLed2 += (char)payload[i];
    statusLed3 += (char)payload[i];
    statusLed4 += (char)payload[i];
    statusFan += (char)payload[i];
  }
  Serial.println(statusLed);
  Serial.println("-----------------------");

  StaticJsonDocument<200> doc;
  deserializeJson(doc, payload, length);

  control_device(topic, LighTopic, statusLed, ledPin);
  control_device(topic, Ligh2Topic, statusLed2, ledPin2);
  control_device(topic, Ligh3Topic, statusLed3, ledPin3);
  control_device(topic, Ligh4Topic, statusLed4, ledPin4);
  control_device(topic, RelayTopic, statusFan, RELAY);

  //  if (String(topic) == RgbTopic) {
  //   // Serial.print("Changing output to ");
  //   setColor(red, green, blue);
  // }
}

// void setColor(int R, int G, int B) {
//   analogWrite(pinRed, R);
//   analogWrite(pinGreen, G);
//   analogWrite(pinBlue, B);
// }

void loop() {
  if (!client.connected()) {
    connect_subscribe_mqtt();
  };
  publish_mqtt();
  publish_mqtt_Fire();
  publish_mqtt_Rain();
  client.loop();
   pinMode(cambien, INPUT);
  //   int value = digitalRead(rainSensor);  //Đọc tín hiệu cảm biến mưa
  // if (value == HIGH) {                  // Cảm biến đang không mưa
  //   Serial.println("Dang khong mua");
  // } else {
  //   Serial.println("Dang mua");
  // } // Đợi 1 tí cho lần kiểm tra tiếp theo. Bạn hãy tham khảo bài "Viết chương trình không dùng làm delay" trên Arduino.VN để kết hợp đoạn code này và cả chương trình của bạn
}
 