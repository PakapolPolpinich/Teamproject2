#include <WiFi.h>
#include <PubSubClient.h>

#include <DHT.h>
#define DHTPIN 4
#define DHTTYPE DHT22

// WiFi
const char *ssid = "xxxx"; // Enter your Wi-Fi name
const char *password = "xxxx";  // Enter Wi-Fi password

const char *mqtt_broker = "xxxx";//broker web
const char *topic = "DataforDht22"; 
const char *mqtt_username = "xxxx";// we create for broker 
const char *mqtt_password = "xxxx";
const int mqtt_port = 6688;


WiFiClient espClient;
PubSubClient client(espClient); //create class method is client you can change client to another name 

DHT dht(DHTPIN, DHTTYPE);

volatile float lastHumidity = 0.0;
volatile float lastTemperature = 0.0;

void ReadSensorTask(void *parameter) {
    while(1){ // Infinite loop
        lastHumidity = dht.readHumidity();
        lastTemperature = dht.readTemperature();
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second
    }
}

void MqttPublishTask(void *parameter) {
  char text_humadity[50];
  char text_temapature[50];
  char sentence[50];
    while(1) { 
        float humidity = lastHumidity;
        float temparature = lastTemperature;
        dtostrf(humidity, 5, 2, text_humadity);
        dtostrf(temparature, 5, 2, text_temapature);
        sprintf(sentence, "{\"temp\":\"%s\",\"hum\":\"%s\"}",text_temapature, text_humadity);
        client.publish(topic,sentence);
        client.loop();
        vTaskDelay(5000 / portTICK_PERIOD_MS); // Delay for 5 seconds
    }
}


void setup() {
    Serial.begin(115200);
    // Connecting to a WiFi network
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.println("Connecting to WiFi..");
    }
    Serial.println("Connected to the Wi-Fi network");
    //connecting to a mqtt broker
    client.setServer(mqtt_broker, mqtt_port);//set broker to connect it not have return value
    client.setCallback(callback);//Subscribe -> when recieve data go to function call back
    while (!client.connected()) {
        String client_id = "esp32-client-"; // name broker it not same who use same broker
        client_id += String(WiFi.macAddress());
        Serial.printf("The client %s connects to the public MQTT broker\n", client_id.c_str());
        if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {//connect with broker (set name,name mqtt,password) pointer parameter char is not use address it return true when finish
            Serial.println("Public EMQX MQTT broker connected");
        } else {
            Serial.print("failed with state ");
            Serial.print(client.state());
            delay(2000);
        }
    }
    dht.begin();
    xTaskCreatePinnedToCore(ReadSensorTask, "Read Sensor Task", 10000, NULL, 1, NULL,0);// use core 0
    xTaskCreatePinnedToCore(MqttPublishTask, "MQTT Publish Task", 10000, NULL, 1, NULL,1);//use core 1
}

void callback(char *topic, byte *payload, unsigned int length) {
    Serial.print("Message arrived in topic: ");
    Serial.println(topic);
    Serial.print("Message:");
    for (int i = 0; i < length; i++) {
        Serial.print((char) payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void loop() {
}
