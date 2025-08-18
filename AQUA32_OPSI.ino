/*
|========================================================================|
| OPSI                                                                   |
| Program AQUA32                                                         |
| Dikembangkan Oleh Tim DeWan                                            |
| Dilarang Keras untuk Mengubah Program yang telah Dibuat oleh Tim kami! |
| Credit: @is_mnw                                                        |
|========================================================================|
*/

// library yang dibutuhkan
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

// konfigurasi ssid dan password
const char* ssid = "AQUA32";
const char* password = "12345678";

WebServer server(80); // port web server

// konstanta
const int pinSensor[5] = {13, 14, 15, 16, 17}; // pin untuk sensor| turb, temp, tds, ph, voltase |
const char* listStatusAir[] = {"Jernih", "Agak keruh", "Keruh", "Sangat Keruh"}; // variabel menyimpan string value antara jernih, agak keruh, keruh, dan sangat keruh
const float VCC = 3.3;
const float ADC = 4095.0;
const float R0 = 10000.0;
 
// variabel
int nilai[5]; // nilai mentah sensor| turb, temp, tds, ph, voltase |
float skor[5]; // skor| turb 1 s.d 10 | temp C | tds ~ppm | ph 0 s.d 14 | voltase V |
String statusAir = listStatusAir[3];

// html web
void handleRoot() {
  String html = "<!DOCTYPE html><html>";
  html += "<head><title>ESP32 Sensor</title></head>";
  html += "<body><h2>Data Sensor dari ESP32</h2>";
  html += "<p>Suhu: " + String(skor[1]) + " C</p>";
  html += "<p>TDS: " + String(skor[2]) + " ppm</p>";
  html += "<p>pH: " + String(skor[3]) + "</p>";
  html += "</body></html>";

  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200); // memulai serial monitor pada baud 115200
  WiFi.softAP(ssid, password);
  Serial.println("Memulai Access Point!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.softAPIP());  // alamat IP : 192.168.4.1
  server.on("/", handleRoot); // buka halaman utama
  server.begin(); // mulai server
  Serial.println("Memulai Web Server!");
}

void loop() {
  server.handleClient(); // proses request dari browser
  // baca nilai sensor
  for (int i = 0; i < 4; i++) {
    nilai[i] = analogRead(pinSensor[i]);
  }

  // rumus turbidity
  skor[0] = (float)(nilai[0] - 900) / (2400.0 - 900.0) * 10.0;
  if (skor[0] < 0) skor[0] = 0;
  if (skor[0] > 10) skor[0] = 10;
  
  // konversi skor menjadi status kejernihan air
  if (skor[0] >= 8) { 
    statusAir = listStatusAir[0];
  } else if (skor[0] < 8 && skor[0] >=6) {
    statusAir = listStatusAir[1];
  } else if (skor[0] < 6 && skor[0] >=3) {
    statusAir = listStatusAir[2];
  } else {
    statusAir = listStatusAir[3];
  }
  
  // rumus suhu
  float vT = nilai[1] * (VCC / ADC);
  float rT = (R0 * (VCC / vT - 1));
  float tK = 1.0 / ((1.0 / 3950) * log(rT / R0) + (1.0 / 298.15));
  skor[1] = tK - 273.15;

  // rumus tds
  float vS = nilai[2] * (VCC / ADC);
  float tS = 1.0 + 0.02 * (skor[1] - 25.0);
  float ec = (133.42 - pow(vS, 3) - 255.86 * pow(vS, 2) + 857.39 * vS) * tS;
  skor[2] = ec * 0.5;

  // rumus sensor ph
  float valuePH = (float)nilai[3] * 3.3 / 4095.0 / 6;
  skor[3] = -5.70 * valuePH + (21.34 + 1.0); // nilai kalibrasi dari sensor menggunakan program yang berbeda

  // rumus tegangan panel surya
  float Vread = (nilai[4] * VCC) / ADC;
  skor[4] = Vread * (30000.0 )
  
  // tampilkan nilai di Serial Monitor, lebih untuk membaca data yang akan ditampilkan
  Serial.print("Kejernihan:");
  Serial.print(statusAir);
  Serial.print("  Skor Kejernihan:");
  Serial.print(int(skor[0]));
  Serial.print("  |   Suhu (C):");
  Serial.print(skor[1], 2);
  Serial.print("  |   PPM:");
  Serial.print(String(int(skor[2])) + "ppm");
  Serial.print("  |   pH:");
  Serial.print(skor[3], 2);
  Serial.print("  |   Voltase Panel Surya:");
  Serial.print(skor[4], 2 + "V");
  Serial.println();
  
  delay(1000); // durasi update data
}
