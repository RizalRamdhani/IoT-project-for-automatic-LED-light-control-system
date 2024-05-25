#include <NTPClient.h>
#include <WiFi.h>
#include <RTClib.h>
#include <Wire.h>
#include <SPI.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

RTC_DS3231 rtc;

// Setup Wifi
const char *ssid = "MAKERINDO2";
const char *password = "makerindo2019";
String jsonDataMasuk;

const long utcOffsetInSeconds = 25200;

// Setting tanggal menjadi nama hari
const char* daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "id.pool.ntp.org", utcOffsetInSeconds);

// MQTT Broker
const char *mqttServer = "broker.hivemq.com";
const int mqttPort = 1883;
WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// Define LED pins
const int merahPin = 18; // Replace with actual pin number
const int hijauPin = 19; // Replace with actual pin number

void callback(char* topic, byte* mesagge, unsigned int length) ;


void setupMQTT()
{
  mqttClient.setServer(mqttServer, mqttPort);
  mqttClient.setCallback(callback);
}


void reconnect()
{
  Serial.println("Connecting to MQTT Broker...");
  while (!mqttClient.connected())
  {
    Serial.println("Reconnecting to MQTT Broker..");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    mqttClient.setBufferSize(2048);
    if (mqttClient.connect(clientId.c_str()))
    {
      Serial.println("Connected.");
      // subscribe to topic
      mqttClient.subscribe("esp32/message123");
    }
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(); // Hanya perlu begin() tanpa parameter karena kita menggunaka
  
  
  // Initialize LED pins
  pinMode(merahPin, OUTPUT);
  pinMode(hijauPin, OUTPUT);
  
  WiFi.begin(ssid, password);

  while ( WiFi.status() != WL_CONNECTED ) {
    delay ( 500 );
    Serial.print ( "." );
  }

  timeClient.begin();
  timeClient.update();
  
  // Kalibrasi Waktu RTC dengan NTP
  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  rtc.adjust(DateTime(timeClient.getEpochTime())); // Set RTC dari NTP
  
  // Connect to MQTT Server
  setupMQTT();
}


void loop() {
 if (!mqttClient.connected())
 {
  reconnect();
  /* code */
 }
 
  
  mqttClient.loop();
  timeClient.update();
  DateTime now = rtc.now();
  
  // Sinkronisasi waktu RTC dengan waktu dari NTP setelah pembaruan waktu dari NTP
  rtc.adjust(DateTime(timeClient.getEpochTime()));
  
  // Parsing JSON
  JsonDocument doc;
  
  deserializeJson(doc, jsonDataMasuk);
  
  // Loop through LED configurations
  JsonArray ledArray = doc["led"];
  for(JsonObject led : ledArray) {
    String waktuNyala = led["waktuNyala"];
    int durasi = led["durasi"];

    // Parsing waktuNyala menjadi jam dan menit
    int jamNyala = waktuNyala.substring(0, 2).toInt();
    int menitNyala = waktuNyala.substring(3).toInt();

    // Menghitung waktuNyala dalam detik
    int waktuNyalaDetik = jamNyala * 3600 + menitNyala * 60;

    // Menghitung waktu dalam detik sejak tengah malam
    int waktuSekarangDetik = now.hour() * 3600 + now.minute() * 60 + now.second();

    // Memeriksa apakah sudah waktunya untuk menyalakan LED
    if (waktuSekarangDetik >= waktuNyalaDetik && waktuSekarangDetik < waktuNyalaDetik + durasi) {
      // Menyalakan LED sesuai konfigurasi
      if (led["namaLed"] == "Merah") {
        digitalWrite(merahPin, HIGH);
      } else if (led["namaLed"] == "Hijau") {
        digitalWrite(hijauPin, HIGH);
      }
    } else {
      // Mematikan LED jika di luar jadwal
      if (led["namaLed"] == "Merah") {
        digitalWrite(merahPin, LOW);
      } else if (led["namaLed"] == "Hijau") {
        digitalWrite(hijauPin, LOW);
      }
    }
  }
    // Menampilkan data waktu NTP
  Serial.print(daysOfTheWeek[timeClient.getDay()]);
  Serial.print(", ");
  Serial.print(timeClient.getHours());
  Serial.print(":");
  Serial.print(timeClient.getMinutes());
  Serial.print(":");
  Serial.println(timeClient.getSeconds());
  delay(1000);
  
  // Menampilkan Data Waktu RTC
  Serial.print("Jam : ");
  Serial.print(now.hour());       // Menampilkan Jam
  Serial.print(":");
  Serial.print(now.minute());     // Menampilkan Menit
  Serial.print(":");
  Serial.println(now.second());     // Menampilkan Detik
}


void callback(char* topic, byte* mesagge, unsigned int length) {
  Serial.print("Message arrived ");
  Serial.print(topic);
  Serial.print("] ");
  jsonDataMasuk = "";
  for (int i=0;i<length;i++) {
    jsonDataMasuk += (char)mesagge[i];
  }
  Serial.println();
}