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
#include "printf.h"

//Library BLE
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

//konfigurasi stack size
SET_LOOP_TASK_STACK_SIZE(64 * 1024); // 64KB

//konfigurasi RTC
RTC_DS3231 rtc;
char days[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//Konfigurasi BLE
#define SERVICE_UUID "17f520e2-b521-4d69-8e1c-88a11fe6e257"
#define CHARACTERISTIC_UUID "c8236411-738f-4b1c-8913-85599c76237f"

//konfigurasi NRF24L01
RF24 radio(4, 5); //(pin CE, pin CSN)
RF24Network network(radio); // Network uses that radio
uint8_t dataBuffer[MAX_PAYLOAD_SIZE];  //MAX_PAYLOAD_SIZE is defined in RF24Network_config.h
#define LED_BUILTIN 2

//alamat node
const uint16_t NODE_Master = 00;   // Address of our node in Octal format
const uint16_t this_node = 01;   // Address of our node in Octal format
const uint16_t NODE_2 = 02;  // Address of the other node in Octal format
const uint16_t NODE_3 = 03;  // Address of the other node in Octal format
const uint16_t NODE_4 = 04;  // Address of the other node in Octal format
const uint16_t NODE_5 = 05;  // Address of the other node in Octal format


//variabel DATA
int ID;
int suhu;
int kelembapan;
int unix1;
int berat;
int unix2;
int pitch;
int roll;
int frekuensi;
int unix3;
int tofx;
int tofy;
int tofz;
int unix4;
int usx;
int usy;
int usz;
int unix5;

struct paket_pesan {  // Structure of our payload
  int ID;
  int suhu;
  int kelembapan;
  int unix1;
  int berat;
  int unix2;
  int pitch;
  int roll;
  int frekuensi;
  int unix3;
  int tofx;
  int tofy;
  int tofz;
  int unix4;
  int usx;
  int usy;
  int usz;
  int unix5;
};

//variabel millis
// unsigned long previousTime = 0; // Waktu sebelumnya
// unsigned long intervalmillis = 10000; // Interval waktu (dalam milidetik)

//variabel RSSI node
int NODE_Master_RSSI;
int NODE_2_RSSI;
int NODE_3_RSSI;
int NODE_4_RSSI;
int NODE_5_RSSI;

//variabel BLE
int scanTime = 1; //In seconds

//Fungsi untuk 2 loop
//TaskHandle_t Task1;

//konfigurasi fungsi scan RSSI BLE
BLEScan* pBLEScan;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      if (advertisedDevice.getName() == "NODE_Master")
      {
        NODE_Master_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_2")
      {
        NODE_2_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_3")
      {
        NODE_3_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_4")
      {
        NODE_4_RSSI = advertisedDevice.getRSSI();
      }
      if (advertisedDevice.getName() == "NODE_5")
      {
        NODE_5_RSSI = advertisedDevice.getRSSI();
      }
      //Serial.printf("Advertised Device: %s \n", advertisedDevice.toString().c_str());
    }
};

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

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  if (! rtc.begin()) {
    Serial.println("Could not find RTC! Check circuit.");
    while (1);
  }

  //fungsi setup untuk NRF24L01
  while (!Serial) {
    // some boards need this because of native USB capability
  }
  Serial.println(F("RF24Network/examples/helloworld_tx/"));

  if (!radio.begin()) {
    Serial.println(F("Radio hardware not responding!"));
    while (1) {
      // hold in infinite loop
    }
  }
  radio.setChannel(90);
  network.begin(/*node address*/ this_node);
  printf_begin();        // needed for RF24* libs' internal printf() calls
  radio.printDetails();  // requires printf support

  //BLE
  BLEDevice::init("NODE_1");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer ->createService(SERVICE_UUID);
  BLECharacteristic *pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE);

  pCharacteristic->setValue("Hello this is NODE 1");
  pService->start();
  BLEDevice::startAdvertising();
  Serial.println("BLE READY!!!");

  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value

  //scan BLE
  Serial.println("SCANNING......");
  BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  Serial.print("RSSI NODE Master : " + String(NODE_Master_RSSI));
  Serial.print(" || RSSI NODE 2 : " + String(NODE_2_RSSI));
  Serial.print(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
  Serial.print(" || RSSI NODE 4 : " + String(NODE_4_RSSI));
  Serial.println(" || RSSI NODE 5 : " + String(NODE_5_RSSI));

  Serial.printf("Arduino Stack was set to %d bytes", getArduinoLoopTaskStackSize());
  // Print unused stack for the task that is running setup()
  Serial.printf("\nSetup() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));

  suhu = 12;
  kelembapan = 99;
}

void loop() {
  delay(1000);

  // Print unused stack for the task that is running loop() - the same as for setup()
  Serial.printf("\nLoop() - Free Stack Space: %d", uxTaskGetStackHighWaterMark(NULL));

  network.update();  // Check the network regularly
  DateTime now = rtc.now();
  StaticJsonDocument<512> doc; // Json Document

  //scan ble
  //unsigned long currentTime = millis(); // Waktu saat ini

  // if (currentTime - previousTime >= intervalmillis) {
  //   previousTime = currentTime; // Perbarui waktu sebelumnya
  //   Serial.println("MODE : SCANNING......");
  //   BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
  //   Serial.print("RSSI NODE 2 : " + String(NODE_2_RSSI));
  //   Serial.print(" || RSSI NODE 3 : " + String(NODE_3_RSSI));
  //   Serial.print(" || RSSI NODE 4 : " + String(NODE_4_RSSI));
  //   Serial.println(" || RSSI NODE 5 : " + String(NODE_5_RSSI));
  // }

  while (network.available()) {
    RF24NetworkHeader header;  // If so, grab it and print it out
    paket_pesan pesan;
    network.read(header, &pesan, sizeof(pesan));  // Get the data
    Serial.print("ID          :");
    Serial.println(pesan.ID);
    Serial.print("Suhu        :");
    Serial.println(pesan.suhu);
    Serial.print("Kelembapan  :");
    Serial.println(pesan.kelembapan);
    Serial.print("Unixtime 1  :");
    Serial.println(pesan.unix1);
    Serial.print("Berat       :");
    Serial.println(pesan.berat);
    berat = pesan.berat;
    Serial.print("Unixtime 2  :");
    Serial.println(pesan.unix2);
    unix2 = pesan.unix2;
    Serial.print("Pitch       :");
    Serial.println(pesan.pitch);
    pitch = pesan.pitch;
    Serial.print("Roll        :");
    Serial.println(pesan.roll);
    roll = pesan.roll;
    Serial.print("Frekuensi   :");
    Serial.println(pesan.frekuensi);
    frekuensi = pesan.frekuensi;
    Serial.print("Unixtime 3  :");
    Serial.println(pesan.unix3);
    unix3 = pesan.unix3;
    Serial.print("Tof X       :");
    Serial.println(pesan.tofx);
    tofx = pesan.tofx;
    Serial.print("Tof Y       :");
    Serial.println(pesan.tofy);
    tofy = pesan.tofy;
    Serial.print("Tof Z       :");
    Serial.println(pesan.tofz);
    tofz = pesan.tofz;
    Serial.print("Unixtime 4  :");
    Serial.println(pesan.unix4);
    unix4 = pesan.unix4;
    Serial.print("US X        :");
    Serial.println(pesan.usx);
    usx = pesan.usx;
    Serial.print("US Y        :");
    Serial.println(pesan.usy);
    usy = pesan.usy;
    Serial.print("US Z        :");
    Serial.println(pesan.usz);
    usz = pesan.usz;
    Serial.print("Unixtime 5  :");
    Serial.println(pesan.unix5);
    unix5 = pesan.unix5;
    //==================================================POSISI NODE KE - 1==================================================
    if (pesan.ID == 6) {
      Serial.print("Received packet from NODE Master");
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_2_RSSI >= NODE_3_RSSI && NODE_2_RSSI >= NODE_4_RSSI && NODE_2_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 1, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          if (NODE_3_RSSI >= NODE_4_RSSI && NODE_3_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 12, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              if (NODE_4_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1234, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 12345, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1235, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 12354, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_4_RSSI >= NODE_3_RSSI && NODE_4_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 12, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              if (NODE_3_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1243, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 12435, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1245, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 12453, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_5_RSSI >= NODE_3_RSSI && NODE_5_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 12, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              if (NODE_3_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1253, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 12534, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_4_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1254, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 12543, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_3_RSSI >= NODE_2_RSSI && NODE_3_RSSI >= NODE_4_RSSI && NODE_3_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 1, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          if (NODE_2_RSSI >= NODE_4_RSSI && NODE_2_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 13, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              if (NODE_4_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1324, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 13245, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1325, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 13254, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_4_RSSI >= NODE_2_RSSI && NODE_4_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 13, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              if (NODE_2_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1342, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 13425, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1345, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 13452, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_5_RSSI >= NODE_2_RSSI && NODE_5_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 13, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              if (NODE_2_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1352, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 13524, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_4_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1354, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 13542, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_2_RSSI && NODE_4_RSSI >= NODE_3_RSSI && NODE_4_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 1, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          if (NODE_2_RSSI >= NODE_3_RSSI && NODE_2_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 14, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              if (NODE_3_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1423, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 14235, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1425, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 14253, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_3_RSSI >= NODE_2_RSSI && NODE_3_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 14, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              if (NODE_2_RSSI >= NODE_5_RSSI) {
                paket_pesan pesan = { 143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1432, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_5);
                  bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                  if (!NODE_5) {
                    paket_pesan pesan = { 14325, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_5_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_5);
                bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
                if (!NODE_5) {
                  paket_pesan pesan = { 1435, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 14352, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_5_RSSI >= NODE_2_RSSI && NODE_5_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 14, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              if (NODE_2_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1452, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 14523, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_3_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1453, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 14532, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_2_RSSI && NODE_5_RSSI >= NODE_3_RSSI && NODE_5_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 1, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          if (NODE_2_RSSI >= NODE_3_RSSI && NODE_2_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 15, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              if (NODE_3_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1523, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 15234, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_4_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1524, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 15243, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_3_RSSI >= NODE_2_RSSI && NODE_3_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 15, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              if (NODE_2_RSSI >= NODE_4_RSSI) {
                paket_pesan pesan = { 153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1532, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_4);
                  bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                  if (!NODE_4) {
                    paket_pesan pesan = { 15324, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_4_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_4);
                bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
                if (!NODE_4) {
                  paket_pesan pesan = { 1534, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 15342, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          } else if (NODE_4_RSSI >= NODE_2_RSSI && NODE_4_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 15, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              if (NODE_2_RSSI >= NODE_3_RSSI) {
                paket_pesan pesan = { 154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_2);
                bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                if (!NODE_2) {
                  paket_pesan pesan = { 1542, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_3);
                  bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                  if (!NODE_3) {
                    paket_pesan pesan = { 15423, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              } else if (NODE_3_RSSI >= NODE_2_RSSI) {
                paket_pesan pesan = { 154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_3);
                bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
                if (!NODE_3) {
                  paket_pesan pesan = { 1543, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                  RF24NetworkHeader header(/*to node*/ NODE_2);
                  bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
                  Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
                  if (!NODE_2) {
                    paket_pesan pesan = { 15432, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                    RF24NetworkHeader header(/*to node*/ NODE_Master);
                    bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                    Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
                  }
                }
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    //==================================================POSISI NODE KE - 2==================================================
    if (pesan.ID == 2) {
      Serial.print("Received packet from NODE 2");
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_3_RSSI >= NODE_4_RSSI && NODE_3_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 21, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          if (NODE_4_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 2134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 21345, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 2135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 21354, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_3_RSSI && NODE_4_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 21, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          if (NODE_3_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 2143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 21435, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 2145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 21453, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_3_RSSI && NODE_5_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 21, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          if (NODE_3_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 2153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 21534, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_4_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 2154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 21543, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 3) {
      Serial.print("Received packet from NODE 3");
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_2_RSSI >= NODE_4_RSSI && NODE_2_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 31, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          if (NODE_4_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 3124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 31245, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 3125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 31254, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_2_RSSI && NODE_4_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 31, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          if (NODE_2_RSSI >= NODE_5_RSSI) {
            paket_pesan pesan = { 314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 3142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 31425, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 3145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 31452, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_2_RSSI && NODE_5_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 31, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          if (NODE_2_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 3152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 31524, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_4_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 3154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 31542, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 4) {
      Serial.print("Received packet from NODE 4");
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_2_RSSI >= NODE_3_RSSI && NODE_2_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 41, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          if (NODE_3_RSSI >= NODE_5_RSSI) {
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 4123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 41235, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 4125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 41253, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_3_RSSI >= NODE_2_RSSI && NODE_3_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 41, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          if (NODE_2_RSSI >= NODE_5_RSSI) {
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 4132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_5);
              bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
              if (!NODE_5) {
                paket_pesan pesan = { 41325, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_5_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_5);
            bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
            if (!NODE_5) {
              paket_pesan pesan = { 4135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 41352, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_2_RSSI && NODE_5_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 41, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          if (NODE_2_RSSI >= NODE_3_RSSI) {
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 4152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 41523, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_3_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 4153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 41532, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 5) {
      Serial.print("Received packet from NODE 5");
      BLEScanResults foundDevices = pBLEScan->start(scanTime, false);
      if (NODE_2_RSSI >= NODE_3_RSSI && NODE_2_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 51, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          if (NODE_3_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 5123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 51234, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_4_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 5124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 51243, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_3_RSSI >= NODE_2_RSSI && NODE_3_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 51, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          if (NODE_2_RSSI >= NODE_4_RSSI) {
            paket_pesan pesan = { 513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 5132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_4);
              bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
              if (!NODE_4) {
                paket_pesan pesan = { 51324, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_4_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_4);
            bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
            if (!NODE_4) {
              paket_pesan pesan = { 5134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 51342, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_2_RSSI && NODE_4_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 51, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          if (NODE_2_RSSI >= NODE_3_RSSI) {
            paket_pesan pesan = { 514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_2);
            bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
            if (!NODE_2) {
              paket_pesan pesan = { 5142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_3);
              bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
              if (!NODE_3) {
                paket_pesan pesan = { 51423, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          } else if (NODE_3_RSSI >= NODE_2_RSSI) {
            paket_pesan pesan = { 514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_3);
            bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
            if (!NODE_3) {
              paket_pesan pesan = { 5143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
              RF24NetworkHeader header(/*to node*/ NODE_2);
              bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
              Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
              if (!NODE_2) {
                paket_pesan pesan = { 51432, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
                RF24NetworkHeader header(/*to node*/ NODE_Master);
                bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
                Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
              }
            }
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    //==================================================POSISI NODE KE - 3==================================================
    if (pesan.ID == 23) {
      if (NODE_4_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 2314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 23145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 2315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 23154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 24) {
      if (NODE_3_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 2413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 24135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 2415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 24153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 25) {
      if (NODE_3_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 2513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 25134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 2514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 25143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 32) {
      if (NODE_4_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 3214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 32145, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 3215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 32154, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 34) {
      if (NODE_2_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 3412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 34125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 3415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 34152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 35) {
      if (NODE_2_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 3512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 35124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 3514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 35142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 42) {
      if (NODE_3_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 4213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 42135, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 4215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 42153, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 43) {
      if (NODE_2_RSSI >= NODE_5_RSSI) {
        paket_pesan pesan = { 431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 4312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_5);
          bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
          if (!NODE_5) {
            paket_pesan pesan = { 43125, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_5_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_5);
        bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
        if (!NODE_5) {
          paket_pesan pesan = { 4315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 43152, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 45) {
      if (NODE_2_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 4512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 45123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_3_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 4513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 45132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 52) {
      if (NODE_3_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 5213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 52134, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 5214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 52143, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 53) {
      if (NODE_2_RSSI >= NODE_4_RSSI) {
        paket_pesan pesan = { 531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 5312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_4);
          bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
          if (!NODE_4) {
            paket_pesan pesan = { 53124, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_4_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_4);
        bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
        if (!NODE_4) {
          paket_pesan pesan = { 5314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 53142, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }
    if (pesan.ID == 54) {
      if (NODE_2_RSSI >= NODE_3_RSSI) {
        paket_pesan pesan = { 541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_2);
        bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
        if (!NODE_2) {
          paket_pesan pesan = { 5412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_3);
          bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
          if (!NODE_3) {
            paket_pesan pesan = { 54123, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      } else if (NODE_3_RSSI >= NODE_2_RSSI) {
        paket_pesan pesan = { 541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_3);
        bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
        if (!NODE_3) {
          paket_pesan pesan = { 5413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
          RF24NetworkHeader header(/*to node*/ NODE_2);
          bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
          Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
          if (!NODE_2) {
            paket_pesan pesan = { 54132, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
            RF24NetworkHeader header(/*to node*/ NODE_Master);
            bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
            Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
          }
          digitalWrite(LED_BUILTIN, HIGH);
          delay(100);
        }
        digitalWrite(LED_BUILTIN, LOW);
      }
    }

    //==================================================POSISI NODE KE - 4==================================================
    if (pesan.ID == 234) {
      paket_pesan pesan = { 2341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 23415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 235) {
      paket_pesan pesan = { 2351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 23514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 243) {
      paket_pesan pesan = { 2431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 24315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 245) {
      paket_pesan pesan = { 2451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 24513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 253) {
      paket_pesan pesan = { 2531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 25314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 254) {
      paket_pesan pesan = { 2541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 25413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 324) {
      paket_pesan pesan = { 3241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 32415, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 325) {
      paket_pesan pesan = { 3251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 32514, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 342) {
      paket_pesan pesan = { 3421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 34215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 345) {
      paket_pesan pesan = { 3451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 34512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 352) {
      paket_pesan pesan = { 3521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 35214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 354) {
      paket_pesan pesan = { 3541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 35412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 423) {
      paket_pesan pesan = { 4231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 42315, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 425) {
      paket_pesan pesan = { 4251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 42513, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 432) {
      paket_pesan pesan = { 4321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_5);
      bool NODE_5 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_5 ? F("DATA TERKIRIM KE NODE 5") : F("GAGAL TERKIRIM KE NODE 5"));
      if (!NODE_5) {
        paket_pesan pesan = { 43215, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 435) {
      paket_pesan pesan = { 4351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 43512, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 452) {
      paket_pesan pesan = { 4521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 45213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 453) {
      paket_pesan pesan = { 4531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 45312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 523) {
      paket_pesan pesan = { 5231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 52314, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 524) {
      paket_pesan pesan = { 5241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 52413, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 532) {
      paket_pesan pesan = { 5321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_4);
      bool NODE_4 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_4 ? F("DATA TERKIRIM KE NODE 4") : F("GAGAL TERKIRIM KE NODE 4"));
      if (!NODE_4) {
        paket_pesan pesan = { 53214, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 534) {
      paket_pesan pesan = { 5341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 53412, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 542) {
      paket_pesan pesan = { 5421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_3);
      bool NODE_3 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_3 ? F("DATA TERKIRIM KE NODE 3") : F("GAGAL TERKIRIM KE NODE 3"));
      if (!NODE_3) {
        paket_pesan pesan = { 54213, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 543) {
      paket_pesan pesan = { 5431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_2);
      bool NODE_2 = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_2 ? F("DATA TERKIRIM KE NODE 2") : F("GAGAL TERKIRIM KE NODE 2"));
      if (!NODE_2) {
        paket_pesan pesan = { 54312, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
        RF24NetworkHeader header(/*to node*/ NODE_Master);
        bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
        Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }

    //==================================================POSISI NODE KE - 5==================================================
    if (pesan.ID == 5432) {
      paket_pesan pesan = { 54321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 5423) {
      paket_pesan pesan = { 54231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 5342) {
      paket_pesan pesan = { 53421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 5324) {
      paket_pesan pesan = { 53241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 5243) {
      paket_pesan pesan = { 52431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 5234) {
      paket_pesan pesan = { 52341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4532) {
      paket_pesan pesan = { 45321, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4523) {
      paket_pesan pesan = { 45231, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4352) {
      paket_pesan pesan = { 43521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4325) {
      paket_pesan pesan = { 43251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4253) {
      paket_pesan pesan = { 42531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 4235) {
      paket_pesan pesan = { 42351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3542) {
      paket_pesan pesan = { 35421, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3524) {
      paket_pesan pesan = { 35241, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3452) {
      paket_pesan pesan = { 34521, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3425) {
      paket_pesan pesan = { 34251, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3254) {
      paket_pesan pesan = { 32541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 3245) {
      paket_pesan pesan = { 32451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2543) {
      paket_pesan pesan = { 25431, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2534) {
      paket_pesan pesan = { 25341, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2453) {
      paket_pesan pesan = { 24531, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2435) {
      paket_pesan pesan = { 24351, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2354) {
      paket_pesan pesan = { 23541, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }
    if (pesan.ID == 2345) {
      paket_pesan pesan = { 23451, suhu, kelembapan, now.unixtime(), berat, unix2, pitch, roll, frekuensi, unix3, tofx, tofy, tofz, unix4, usx, usy, usz, unix5 };
      RF24NetworkHeader header(/*to node*/ NODE_Master);
      bool NODE_Master = network.write(header, &pesan, sizeof(pesan));
      Serial.println(NODE_Master ? F("DATA TERKIRIM KE NODE Master") : F("GAGAL TERKIRIM KE NODE Master"));
      if (!NODE_Master) {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(100);
      }
      digitalWrite(LED_BUILTIN, LOW);
    }

  }
  //pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
}
