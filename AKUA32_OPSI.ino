/*
|========================================================================|
| OPSI                                                                   |
| Program AKUA32                                                         |
| Dikembangkan Oleh Tim DeWan                                            |
| Credit: -isMnw                                                         |
|========================================================================|
*/

// library yang dibutuhkan
#include <WiFi.h>
#include <WebServer.h>
#include "esp_wifi.h"

// konfigurasi ssid dan password
const char* ssid = "AKUA32";
const char* password = "12345678";

WebServer server(80); // port web server

// konstanta
const int pinSensor[5] = {36, 39, 34, 35, 32}; // pin untuk sensor| turb, temp, tds, ph, voltase |
const char* listStatusAir[] = {"Jernih", "Agak keruh", "Keruh", "Sangat Keruh"}; // variabel menyimpan string value antara jernih, agak keruh, keruh, dan sangat keruh
const float VCC = 3.3;
const float ADC = 4095.0;
const float R0 = 10000.0;
 
// variabel
int nilai[5]; // nilai mentah sensor| turb, temp, tds, ph, voltase |
float skor[5]; // skor| turb 1 s.d 10 | temp C | tds ~ppm | ph 0 s.d 14 | voltase V |
String statusAir = listStatusAir[3];

// fungsi untuk format html untuk web
void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>AQUA32 Sensor Monitor</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      background-color: #121212;
      color: #f1f1f1;
      margin: 0;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
    }

    header {
      border-bottom-left-radius: 15px;
      border-bottom-right-radius: 15px;
      background: #1f1f1f;
      padding: 15px;
      text-align: center;
      font-size: 26px;
      font-weight: bold;
      color: #ffcc00;
      box-shadow: 0 4px 8px rgba(82, 82, 82, 0.4);
      position: relative;
    }

    #title {
      text-decoration: underline;
    }

    #datetime {
      position: absolute;
      right: 15px;
      top: 50%;
      transform: translateY(-50%);
      font-size: 14px;
      color: #cccccc;
    }

    @media (max-width: 600px) {
      #datetime {
        position: static;
        display: block;
        margin-top: 5px;
        text-align: center;
        transform: none;
      }
    }

    main {
      flex: 1;
      display: flex;
      flex-direction: column;
      align-items: center;
      padding: 20px;
    }

    .info {
      margin: 10px;
      font-size: 16px;
      color: #cccccc;
    }

    .cards {
      display: grid;
      grid-gap: 30px;

    }

    @media (max-width: 600px) {
      .cards {
        grid-template-columns: 1fr;
      }
    }

    @media (min-width: 601px) and (max-width: 1024px) {
      .cards {
        grid-template-columns: repeat(2, 1fr);
      }
    }

    @media (min-width: 1025px) {
      .cards {
        grid-template-columns: repeat(5, 1fr);
      }
    }

    .card {
      padding: 15px;
      border-radius: 12px;
      background: #1f1f1f;
      box-shadow: 0 4px 8px rgba(82, 82, 82, 0.4);
      text-align: center;
      min-width: 300px;
    }

    .label {
      font-size: 14px;
      color: #aaaaaa;
    }

    .value {
      font-size: 24px;
      margin-top: 8px;
      color: #ffcc00;
    }

    footer {
      border-top-left-radius: 15px;
      border-top-right-radius: 15px;
      background: #1f1f1f;
      text-align: center;
      padding: 10px;
      font-size: 14px;
      color: #ffcc00;
      box-shadow: 0 -4px 8px rgba(82, 82, 82, 0.4);
    }
  </style>
</head>
<body>
  <header>
    <div id="title">AKUA32 Sensor Monitor</div>
    <div id="datetime"></div>
  </header>

  <main>
    <div class="info"><i id="status">Menyambungkan ke AKUA32...</i></div>

    <div class="cards">
      <div class="card">
        <div class="label">Suhu</div>
        <div class="value" id="suhu">--</div>
      </div>

      <div class="card">
        <div class="label">TDS</div>
        <div class="value" id="tds">--</div>
      </div>

      <div class="card">
        <div class="label">pH</div>
        <div class="value" id="ph">--</div>
      </div>

      <div class="card">
        <div class="label">Voltase</div>
        <div class="value" id="voltase">--</div>
      </div>

      <div class="card">
        <div class="label">Kejernihan</div>
        <div class="value" id="kejernihan">--</div>
      </div>

    </div>
  </main>

  <footer onclick="return confirm('credit oleh tim DeWan')"><i>copyright © Tim DeWan</i></footer>

  <script>
    function updateTime() {
      let now = new Date();
      let options = { weekday: 'short', year: 'numeric', month: 'short', day: 'numeric' };
      let date = now.toLocaleDateString('id-ID', options);
      let time = now.toLocaleTimeString('id-ID');
      document.getElementById("datetime").innerHTML ="| " + date + " " + time +" |";
      }
      setInterval(updateTime, 1000);
      updateTime();

    function updateData() {
      fetch("/data")
      .then(response => response.json())
      .then(data => {
        document.getElementById("kejernihan").innerHTML = data.skalakejernihan + " : " + data.tingkatkejernihan;
        document.getElementById("suhu").innerHTML = data.suhu + " °C";
        document.getElementById("tds").innerHTML = data.tds + " ppm";
        document.getElementById("ph").innerHTML = data.ph;
        document.getElementById("voltase").innerHTML = data.voltase + " V";
        //menunjukan kalau ESP32 terhubung dengan web
        document.getElementById("status").innerHTML = data.status";
      });
    }
    setInterval(updateData, 500);
  </script>
 /body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}
// fungsi untuk update data dari sensor ke web html
void handleData() {
  String data = "{";
  data += "\"tingkatkejernihan\":\"" + statusAir + "\",";
  data += "\"skalakejernihan\":" + String(int(skor[0])) + ",";
  data += "\"suhu\":" + String(skor[1]) + ",";
  data += "\"tds\":" + String(int(skor[2])) + ",";
  data += "\"ph\":" + String(skor[3]) + ",";
  data += "\"voltase\":" + String(skor[4]);
  data += "\"status\":" + String("Terhubung dengan AKUA32 ✔");
  data += "}";
  server.send(200, "application/json", data);
}

void setup() {
  Serial.begin(115200); // memulai serial monitor pada baud 115200
  WiFi.softAP(ssid, password);
  Serial.println("Memulai Access Point!");
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.softAPIP());  // memulai alamat IP : 192.168.4.1
  server.on("/", handleRoot); // buka halaman utama
  server.on("/data", handleData);
  server.begin(); // mulai server
  Serial.println("Memulai Web Server!");
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  server.handleClient(); // proses request dari browser
  // baca nilai sensor
  for (int i = 0; i <= 4; i++) {
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
  skor[3] = -5.70 * valuePH + (21.34 + 1.5); // nilai kalibrasi dari sensor menggunakan program yang berbeda

  // rumus tegangan panel surya
  float Vread = (nilai[4] * VCC) / ADC;
  skor[4] = Vread * (30000.0 + 7500.0) / 7500.0;
  
  // tampilkan nilai di Serial Monitor, lebih untuk membaca data yang akan ditampilkan
  Serial.print(nilai[0]);
  Serial.print("|");
  Serial.print(int(skor[0]));
  Serial.print("|");
  Serial.print(statusAir);
  Serial.print("|");
  Serial.print(nilai[1]);
  Serial.print("|");
  Serial.print(skor[1],2);
  Serial.print("|");
  Serial.print(nilai[2], 2);
  Serial.print("|");
  Serial.print(int(skor[2]));
  Serial.print("|");
  Serial.print(nilai[3]);
  Serial.print("|");  
  Serial.print(skor[3], 2);
  Serial.print("|");
  Serial.print(nilai[4]);
  Serial.print("|");
  Serial.print(skor[4, 2]);
  Serial.println();
  digitalWrite(LED_BUILTIN, HIGH);
  delay(50);
  digitalWrite(LED_BUILTIN, LOW);
  delay(950); // durasi update data
}
