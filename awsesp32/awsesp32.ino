#include "SPIFFS.h"
#include <WiFiClientSecure.h>
#include <Wire.h>

#include <PubSubClient.h>

#define ADC_VREF_mV    3300.0 // in millivolt
#define ADC_RESOLUTION 4096.0
#define PIN_LM35       36 // ESP32 pin GIOP36 (ADC0) connected to LM35

// Enter your WiFi ssid and password
const char* ssid = "wifi name"; //Provide your SSID
const char* password = "wifi password"; // Provide Password
const char* mqtt_server = "xxxxxxxxxxxxxxxxxxxxxxxxxx"; // Relace with your MQTT END point
const int mqtt_port = 8883;

String Read_rootca;
String Read_cert;
String Read_privatekey;

#define BUFFER_LEN 256
long lastMsg = 0;
char msg[BUFFER_LEN];
int value = 0;
byte mac[6];
char mac_Id[18];
int count = 1;

WiFiClientSecure espClient;
PubSubClient client(espClient);

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}
void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");
      // Once connected, publish an announcement...
      client.publish("ei_out", "hello world");
      // ... and resubscribe
      client.subscribe("ei_in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup{
Serial.begin(9600);
Serial.setDebugOutput(true);
pinMode(2,OUTPUT);
setup_wifi();
delay(1000);
if (!SPIFFS.begin(true)) {
  Serial.println("An Error has occurred while mounting SPIFFS");
  return;
}

//=======================================
  //Root CA File Reading.
  File file2 = SPIFFS.open("/AmazonRootCA1.pem", "r");
  if (!file2) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("Root CA File Content:");
  while (file2.available()) {
    Read_rootca = file2.readString();
    Serial.println(Read_rootca);
  }
  //=============================================
  // Cert file reading
  File file4 = SPIFFS.open("/83e2fa1863-certificate.pem.crt", "r");
  if (!file4) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("Cert File Content:");
  while (file4.available()) {
    Read_cert = file4.readString();
    Serial.println(Read_cert);
  }
  //=================================================
  //Privatekey file reading
  File file6 = SPIFFS.open("/83e2fa1863-private.pem.key", "r");
  if (!file6) {
    Serial.println("Failed to open file for reading");
    return;
  }
  Serial.println("privateKey File Content:");
  while (file6.available()) {
    Read_privatekey = file6.readString();
    Serial.println(Read_privatekey);
  }
  //=====================================================

char* pRead_rootca;
  pRead_rootca = (char *)malloc(sizeof(char) * (Read_rootca.length() + 1));
  strcpy(pRead_rootca, Read_rootca.c_str());

  char* pRead_cert;
  pRead_cert = (char *)malloc(sizeof(char) * (Read_cert.length() + 1));
  strcpy(pRead_cert, Read_cert.c_str());

  char* pRead_privatekey;
  pRead_privatekey = (char *)malloc(sizeof(char) * (Read_privatekey.length() + 1));
  strcpy(pRead_privatekey, Read_privatekey.c_str());  

Serial.println("================================================================================================");
Serial.println("Certificates that passing to espClient Method");
Serial.println();
Serial.println("Root CA:");
Serial.write(pRead_rootca);
Serial.println("================================================================================================");
Serial.println();
Serial.println("Cert:");
Serial.write(pRead_cert);
Serial.println("================================================================================================");
Serial.println();
Serial.println("privateKey:");
Serial.write(pRead_privatekey);
Serial.println("================================================================================================");

espClient.setCACert(pRead_rootca);
espClient.setCertificate(pRead_cert);
espClient.setPrivateKey(pRead_privatekey);

client.setServer(mqtt_server, mqtt_port);
client.setCallback(callback);

WiFi.macAddress(mac);
snprintf(mac_Id, sizeof(mac_id), "%02x:%02x:%02x:%02x:%02x", mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
Serial.print(mac_Id);
}

void loop(){
  // read the ADC value from the temperature sensor
  int adcVal = analogRead(PIN_LM35);
  // convert the ADC value to voltage in millivolt
  float milliVolt = adcVal * (ADC_VREF_mV / ADC_RESOLUTION);
  // convert the voltage to the temperature in °C
  float tempC = milliVolt / 10;
  // convert the °C to °F
  float tempF = tempC * 9 / 5 + 32;

  // print the temperature in the Serial Monitor:
  Serial.print("Temperature: ");
  Serial.print(tempC);   // print the temperature in °C
  Serial.print("°C");
  Serial.print("  ~  "); // separator between °C and °F
  Serial.print(tempF);   // print the temperature in °F
  Serial.println("°F");

  delay(500);

if (!client.connected()) {
  reconnect();
}
client.loop();

long now = millis();
  if (now - lastMsg > 2000) {
    lastMsg = now;
    //=============================================================================================
    String macIdStr = mac_Id;
    String temperatureC = String(tempC);
    String temperatureF = String(tempF);
    snprintf (msg, BUFFER_LEN, "{\"mac_Id\" : \"%s\", \"temperatureC\" : %s, \"temperatureF\" : \"%s\"}", macIdStr.c_str(), temperatureC.c_str(), temperatureF.c_str());
    Serial.print("Publish message: ");
    Serial.print(count);
    Serial.println(msg);
    client.publish("ei_out", msg);
    count = count + 1;
    //================================================================================================
  }
  digitalWrite(2, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(1000);                       // wait for a second
  digitalWrite(2, LOW);    // turn the LED off by making the voltage LOW
  delay(1000);                       // wait for a second
}
