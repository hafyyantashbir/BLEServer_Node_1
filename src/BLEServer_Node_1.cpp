#include <Arduino.h>

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
SET_LOOP_TASK_STACK_SIZE(64*1024); // 64KB

//konfigurasi RTC
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//konfigurasi NRF24L01
RF24 radio(4, 5); //(pin CE, pin CSN)
RF24Network network(radio); // Network uses that radio
RF24Mesh mesh(radio, network);
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h

//alamat node
#define this_node 1


//variabel DATA
int node_asal = 1; //data node
unsigned long suhu = 12;  //data suhu
unsigned long kelembapan = 90; //data kelembapan
String datakirim; //data Json yang akan dikirim

//variabel millis
unsigned long currentMillis = 0;

//Fungsi untuk 2 loop
//TaskHandle_t Task1;

//program loop 2
//void loop2( void * parameter) {
//  for (;;) {
//    unsigned long currentTime = millis(); // Waktu saat ini
//
//    if (currentTime - previousTime >= intervalmillis) {
//      previousTime = currentTime; // Perbarui waktu sebelumnya
//      Serial.println("MODE : SCANNING......");
//      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
//      Serial.print("RSSI NODE 2 : " + String(NODE_2_RSSI));
//      Serial.println(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
//    }
//  }
//}

void setup() {
  Serial.begin(115200);

  //Fungsi untuk 2 loop
  //  xTaskCreatePinnedToCore(
  //    loop2,
  //    "BLE_SCANNING",
  //    1000,
  //    NULL,
  //    1,
  //    &Task1,
  //    0);

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

  // print memori stack keseluruhan
  Serial.printf("Arduino Stack was set to %d bytes", getArduinoLoopTaskStackSize());
  // print sisa memori stack pada void setup
  Serial.printf("\nSetup() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));
}

void loop() {
  delay(1000);

  // print sisa memori stack pada void loop
  Serial.printf("\nLoop() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));

  mesh.update();
  DateTime now = rtc.now();
  StaticJsonDocument<96> doc; // Json Document

  // Mengirim data ke master
  if (millis() - currentMillis >= 1000) {
    currentMillis = millis();
    doc["NodeID"] = node_asal;
    doc["Suhu"] = suhu;
    doc["Kelembapan"] = kelembapan;
    doc["Unixtime"] = now.unixtime();
    datakirim = "";
    serializeJson(doc, datakirim);
    char kirim_loop[datakirim.length() + 1];
    datakirim.toCharArray(kirim_loop, sizeof(kirim_loop));

    if (!mesh.write(&kirim_loop, 'M', sizeof(kirim_loop))) {
      if (!mesh.checkConnection()) {
        Serial.println("Memperbaharui Alamat");
        if (mesh.renewAddress() == MESH_DEFAULT_ADDRESS) {
          mesh.begin();
        }
      } else {
        Serial.println("Gagal Mengirim ke Master, Tes jaringan OK");
      }
    } else {
      Serial.print("Berhasil Mengirim ke Master : ");
      Serial.println(datakirim);
    }
  }
}
