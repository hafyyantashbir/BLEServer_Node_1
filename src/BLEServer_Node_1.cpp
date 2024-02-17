#include <Arduino.h>

//library DHT
#include "DHT.h"

//library RTC
#include <Wire.h>
#include "RTClib.h"

//library Json
#include <ArduinoJson.h>

//Librari NRF24L01
#include <SPI.h>
#include <RF24.h>
#include <RF24Network.h>
#include <RF24Mesh.h>
#include "printf.h"

//konfigurasi stack size
SET_LOOP_TASK_STACK_SIZE(32*1024); // 64KB

//konfigurasi RTC
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//konfigurasi NRF24L01
RF24 radio(4, 5); //(pin CE, pin CSN)
RF24Network network(radio); // Network uses that radio
RF24Mesh mesh(radio, network);
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h

//konfigurasi DHT
#define DHTPIN 13
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

//alamat node
#define this_node 1

//variabel DATA
int node_asal = 1; //data node
float suhu;  //data suhu
float kelembapan; //data kelembapan
String datakirim; //data Json yang akan dikirim

//variabel millis
unsigned long currentMillis = 0;

//Fungsi untuk 2 loop
TaskHandle_t Task1;

//program loop 2
void bacasensor( void * parameter) {
 for (;;) {
    suhu = dht.readTemperature();
    kelembapan = dht.readHumidity();
    Serial.println("Running on Core : "+String(xPortGetCoreID())+"Suhu : "+String(suhu)+", Kelembapan : "+String(kelembapan));
 }
}

void setup() {
  Serial.begin(115200);

  dht.begin();

  if (! rtc.begin()) {
    Serial.println("Tidak dapat menemukan RTC! Periksa sirkuit.");
    while (1);
  }

  //fungsi setup untuk NRF24L01
  while (!Serial) {
    // some boards need this because of native USB capability
  }
  mesh.setNodeID(this_node); //Set the Node ID
  Serial.println(F("Menghubungkan ke jaringan..."));

  if (!mesh.begin()){
    if (radio.isChipConnected()){
      do {
        // mesh.renewAddress() will return MESH_DEFAULT_ADDRESS on failure to connect
        Serial.println(F("Gagal terhubung ke jaringan.\nMenghubungkan ke jaringan..."));
      } while (mesh.renewAddress() == MESH_DEFAULT_ADDRESS);
    } else {
      Serial.println(F("NRF24L01 tidak merespon."));
      while (1) {
        // hold in an infinite loop
      }
    }
  }
  printf_begin();
  radio.printDetails();  // print detail konfigurasi NRF24L01

  //Fungsi untuk 2 loop
   xTaskCreatePinnedToCore(
     bacasensor,    /* Task function. */
     "baca_sensor", /* name of task. */
     32768,         /* Stack size of task */
     NULL,          /* parameter of the task */
     1,             /* priority of the task */
     &Task1,        /* Task handle to keep track of created task */
     0);            /* pin task to core 0 */

  // print memori stack keseluruhan
  Serial.printf("Arduino Stack was set to %d bytes", getArduinoLoopTaskStackSize());
  // print sisa memori stack pada void setup
  Serial.printf("\nSetup() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));
}

void loop() {
  // print sisa memori stack pada void loop
  //Serial.printf("\nLoop() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));

  mesh.update();
  DateTime now = rtc.now();
  StaticJsonDocument<128> doc; // Json Document

  // Mengirim data ke master
  if (millis() - currentMillis >= 250) {
    currentMillis = millis();
    doc["NodeID"] = String(node_asal);
    doc["Suhu"] = String(suhu);
    doc["Kelembapan"] = String(kelembapan);
    doc["Unixtime"] = String(now.unixtime());
    datakirim = "";
    serializeJson(doc, datakirim);
    Serial.println(datakirim);
    char kirim_loop[datakirim.length() + 1];
    datakirim.toCharArray(kirim_loop, sizeof(kirim_loop));

    if (!mesh.write(&kirim_loop, '1', sizeof(kirim_loop))) {
      if (!mesh.checkConnection()) {
        Serial.println("Memperbaharui Alamat");
        if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS) {
          mesh.begin();
        }
      } else {
        Serial.println("Gagal Mengirim ke Master, Tes jaringan OK");
      }
    } else {
      Serial.print(" || Berhasil Mengirim ke Master : ");
      Serial.println(datakirim);
    }
  }
}
