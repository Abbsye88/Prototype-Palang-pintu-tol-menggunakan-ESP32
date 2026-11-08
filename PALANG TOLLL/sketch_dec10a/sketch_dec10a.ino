#include <ESP32Servo.h>
#include <WiFi.h>
#include <WebServer.h>

const char* ssid = "Vivo Y17s";
const char* password = "Aditiaaaa";

const int trigPin = 25;
const int echoPin = 32;

const int servoPin = 33;

const int ledPin = 26;
const int buzzerPin = 27;

Servo gerbang;
WebServer server(80);

long duration;
int distance;

unsigned long lastDetectedTime = 0;
unsigned long firstDetectedTime = 0;
unsigned long alertStartTime = 0;

bool gateOpen = false;
bool detecting = false;
bool alertActive = false;
bool alertUsed = false;
bool wifiConnected = false;

int totalDetections = 0;
unsigned long totalResponseTime = 0;
int responseCount = 0;

const char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>Sistem Parkir Otomatis</title>
<style>
* {
  margin: 0;
  padding: 0;
  box-sizing: border-box;
}
body {
  font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
  background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
  min-height: 100vh;
  padding: 20px;
}
.container {
  max-width: 1200px;
  margin: 0 auto;
}
.header {
  text-align: center;
  color: white;
  margin-bottom: 30px;
}
.header h1 {
  font-size: 2.5em;
  margin-bottom: 10px;
  text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
}
.header p {
  font-size: 1.1em;
  opacity: 0.9;
}
.cards {
  display: grid;
  grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
  gap: 20px;
  margin-bottom: 30px;
}
.card {
  background: white;
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.2);
  transition: transform 0.3s;
}
.card:hover {
  transform: translateY(-5px);
}
.card-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 15px;
}
.card-icon {
  font-size: 2em;
}
.card-label {
  font-size: 0.85em;
  color: #666;
  text-transform: uppercase;
  letter-spacing: 1px;
  font-weight: 600;
}
.card-value {
  font-size: 3em;
  font-weight: bold;
  color: #333;
  margin: 10px 0;
}
.card-unit {
  font-size: 0.9em;
  color: #999;
}
.card-blue { border-left: 5px solid #3498db; }
.card-purple { border-left: 5px solid #9b59b6; }
.card-orange { border-left: 5px solid #e67e22; }
.card-green { border-left: 5px solid #27ae60; }
.card-red { border-left: 5px solid #e74c3c; }
.status-bar {
  background: white;
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.2);
  margin-bottom: 30px;
}
.status-content {
  display: flex;
  align-items: center;
  gap: 20px;
}
.status-icon {
  font-size: 2.5em;
}
.status-text h3 {
  font-size: 1.3em;
  margin-bottom: 5px;
  color: #333;
}
.status-text p {
  color: #666;
  font-size: 0.95em;
}
.status-badge {
  margin-left: auto;
  padding: 10px 20px;
  border-radius: 25px;
  font-weight: bold;
  font-size: 0.9em;
}
.badge-active {
  background: #ffeaa7;
  color: #d63031;
}
.badge-idle {
  background: #dfe6e9;
  color: #636e72;
}
.alert-active {
  background: linear-gradient(135deg, #fdcb6e 0%, #e17055 100%);
  animation: pulse 2s infinite;
}
@keyframes pulse {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.8; }
}
.history {
  background: white;
  border-radius: 15px;
  padding: 25px;
  box-shadow: 0 10px 30px rgba(0,0,0,0.2);
}
.history h2 {
  color: #333;
  margin-bottom: 20px;
  font-size: 1.5em;
}
table {
  width: 100%;
  border-collapse: collapse;
}
th {
  background: #667eea;
  color: white;
  padding: 12px;
  text-align: left;
  font-weight: 600;
  text-transform: uppercase;
  font-size: 0.85em;
  letter-spacing: 0.5px;
}
td {
  padding: 12px;
  border-bottom: 1px solid #eee;
  color: #333;
}
tr:hover {
  background: #f8f9fa;
}
.live-indicator {
  display: inline-block;
  width: 10px;
  height: 10px;
  background: #27ae60;
  border-radius: 50%;
  margin-right: 8px;
  animation: blink 1s infinite;
}
@keyframes blink {
  0%, 100% { opacity: 1; }
  50% { opacity: 0.3; }
}
footer {
  text-align: center;
  color: white;
  margin-top: 30px;
  font-size: 0.9em;
  opacity: 0.8;
}
</style>
</head>
<body>
<div class="container">
  <div class="header">
    <h1>🚗 Sistem Parkir Otomatis</h1>
    <p>Real-time Monitoring Dashboard ESP32</p>
  </div>

  <div class="cards">
    <div class="card card-blue">
      <div class="card-header">
        <span class="card-icon">📡</span>
        <span class="card-label"><span class="live-indicator"></span>Live</span>
      </div>
      <div class="card-label">Jarak Deteksi</div>
      <div class="card-value" id="distance">-</div>
      <div class="card-unit">centimeter</div>
    </div>

    <div class="card card-purple">
      <div class="card-header">
        <span class="card-icon">📊</span>
        <span class="card-label">Total</span>
      </div>
      <div class="card-label">Mobil Terdeteksi</div>
      <div class="card-value" id="total">0</div>
      <div class="card-unit">kendaraan</div>
    </div>

    <div class="card card-orange">
      <div class="card-header">
        <span class="card-icon">⏱️</span>
        <span class="card-label">Rata-rata</span>
      </div>
      <div class="card-label">Waktu Respon</div>
      <div class="card-value" id="avgtime">0.0</div>
      <div class="card-unit">detik</div>
    </div>

    <div class="card" id="gateCard">
      <div class="card-header">
        <span class="card-icon" id="gateIcon">🚪</span>
        <span class="card-label">Status</span>
      </div>
      <div class="card-label">Kondisi Gerbang</div>
      <div class="card-value" id="gate">TUTUP</div>
      <div class="card-unit" id="gateDesc">Gerbang tertutup</div>
    </div>
  </div>

  <div class="status-bar" id="statusBar">
    <div class="status-content">
      <span class="status-icon" id="statusIcon">✓</span>
      <div class="status-text">
        <h3 id="statusTitle">Sistem Standby</h3>
        <p id="statusDesc">Menunggu kendaraan masuk zona deteksi (< 15cm)</p>
      </div>
      <div class="status-badge badge-idle" id="statusBadge">IDLE</div>
    </div>
  </div>

  <div class="history">
    <h2>📋 Data Real-time</h2>
    <table>
      <thead>
        <tr>
          <th>Waktu</th>
          <th>Jarak (cm)</th>
          <th>Status Deteksi</th>
          <th>Status Gerbang</th>
        </tr>
      </thead>
      <tbody id="historyTable">
      </tbody>
    </table>
    <h3>Waktu Respon</h3>
    <table>
      <thead>
        <tr>
          <th>Waktu Mulai</th>
          <th>Waktu Selesai</th>
          <th>Lama (detik)</th>
        </tr>
      </thead>
      <tbody id="responseTable"></tbody>
    </table>
  </div>

  <footer>
    🔧 ESP32 Parking System | Sensor: HC-SR04 | Aktuator: Servo SG90
  </footer>
</div>

<script>
let historyData = [];
let wasDetecting = false;
let detectStartTime = 0;


function updateData() {
  fetch('/data')
    .then(response => response.json())
    .then(data => {
      // Update jarak
      document.getElementById('distance').textContent = data.distance;
      
      // Update total deteksi
      document.getElementById('total').textContent = data.totalDetections;
      
      // Update waktu respon rata-rata
      document.getElementById('avgtime').textContent = data.avgResponseTime.toFixed(1);
      
      // Update status gerbang
      const gateCard = document.getElementById('gateCard');
      const gateIcon = document.getElementById('gateIcon');
      const gateText = document.getElementById('gate');
      const gateDesc = document.getElementById('gateDesc');
      
      if (data.gateOpen) {
        gateCard.className = 'card card-green';
        gateIcon.textContent = '🔓';
        gateText.textContent = 'BUKA';
        gateDesc.textContent = 'Gerbang terbuka';
      } else {
        gateCard.className = 'card card-red';
        gateIcon.textContent = '🔒';
        gateText.textContent = 'TUTUP';
        gateDesc.textContent = 'Gerbang tertutup';
      }
      
      // Update status bar
      const statusBar = document.getElementById('statusBar');
      const statusIcon = document.getElementById('statusIcon');
      const statusTitle = document.getElementById('statusTitle');
      const statusDesc = document.getElementById('statusDesc');
      const statusBadge = document.getElementById('statusBadge');
      
      if (data.detecting) {
        statusBar.className = 'status-bar alert-active';
        statusIcon.textContent = '⚠️';
        statusTitle.textContent = 'KENDARAAN TERDETEKSI';
        statusDesc.textContent = 'Jarak: ' + data.distance + 'cm - Gerbang akan ' + 
          (data.gateOpen ? 'menutup setelah kendaraan lewat' : 'terbuka dalam 2 detik');
        statusBadge.className = 'status-badge badge-active';
        statusBadge.textContent = 'AKTIF';
      } else {
        statusBar.className = 'status-bar';
        statusIcon.textContent = '✓';
        statusTitle.textContent = 'Sistem Standby';
        statusDesc.textContent = 'Menunggu kendaraan masuk zona deteksi (< 15cm)';
        statusBadge.className = 'status-badge badge-idle';
        statusBadge.textContent = 'IDLE';
      }
      
      // Update history table
      const now = new Date();
      const timeStr = now.getHours().toString().padStart(2,'0') + ':' + 
                      now.getMinutes().toString().padStart(2,'0') + ':' + 
                      now.getSeconds().toString().padStart(2,'0');
      
if (data.detecting && !wasDetecting) {
    // simpan waktu mulai deteksi
    detectStartTime = Date.now();
    wasDetecting = true;

    // MASUKKAN 1x KE HISTORY
    historyData.unshift({
        time: timeStr,
        distance: data.distance,
        detecting: 'TERDETEKSI (PERTAMA)',
        gate: data.gateOpen ? 'BUKA' : 'TUTUP'
    });

    if (historyData.length > 10) historyData.pop();
}

    // KETIKA SUDAH TIDAK TERDETEKSI LAGI
    if (!data.detecting && wasDetecting) {
        wasDetecting = false;

        let startTime = new Date(detectStartTime);
        let endTime = new Date();
        let duration = ((endTime - startTime) / 1000).toFixed(1);

        // MASUKKAN KE TABEL WAKTU RESPON
        const responseTbody = document.getElementById('responseTable');
        const row = `
            <tr>
                <td>${startTime.toLocaleTimeString()}</td>
                <td>${endTime.toLocaleTimeString()}</td>
                <td><strong>${duration}</strong> detik</td>
            </tr>
        `;
        responseTbody.innerHTML = row + responseTbody.innerHTML;
    }
      const tbody = document.getElementById('historyTable');
      tbody.innerHTML = historyData.map(row => 
        '<tr>' +
        '<td>' + row.time + '</td>' +
        '<td><strong>' + row.distance + ' cm</strong></td>' +
        '<td>' + row.detecting + '</td>' +
        '<td>' + row.gate + '</td>' +
        '</tr>'
      ).join('');
    })
    .catch(err => console.error('Error:', err));
}

// Update setiap 500ms
setInterval(updateData, 500);
updateData();
</script>
</body>
</html>
)=====";

void setup() {
  Serial.begin(115200);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(ledPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);

  digitalWrite(ledPin, LOW);
  digitalWrite(buzzerPin, LOW);

  gerbang.attach(servoPin, 500, 2400);
  gerbang.write(0);

  Serial.println();
  Serial.print("Mencoba koneksi ke WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);

  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  
  Serial.println();
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("✓ WiFi TERHUBUNG!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Buka browser dan akses IP di atas");
  
    server.on("/", HTTP_GET, []() {
      server.send_P(200, "text/html", webpage);
    });

    server.on("/data", HTTP_GET, []() {
      float avgTime = responseCount > 0 ? (float)totalResponseTime / responseCount / 1000.0 : 0.0;
      
      String json = "{";
      json += "\"distance\":" + String(distance) + ",";
      json += "\"gateOpen\":" + String(gateOpen ? "true" : "false") + ",";
      json += "\"detecting\":" + String(detecting ? "true" : "false") + ",";
      json += "\"totalDetections\":" + String(totalDetections) + ",";
      json += "\"avgResponseTime\":" + String(avgTime, 1);
      json += "}";
      
      server.send(200, "application/json", json);
    });

    server.begin();
    Serial.println("Web server started");
  } else {
    wifiConnected = false;
    Serial.println("✗ WiFi TIDAK TERHUBUNG");
    Serial.println("Mode STANDALONE - Sistem tetap berfungsi tanpa dashboard web");
    Serial.println("Sensor & Servo akan bekerja normal");
    WiFi.mode(WIFI_OFF); 
  }
  
  Serial.println("\n=================================");
  Serial.println("SISTEM PARKIR SIAP!");
  Serial.println("=================================\n");
}

int getDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH, 20000);
  if (duration == 0) return 999;

  return duration * 0.034 / 2;
}

void loop() {
  if (wifiConnected) {
    server.handleClient();
  }
  
  distance = getDistance();
  unsigned long currentTime = millis();

  Serial.print("Jarak: ");
  Serial.print(distance);
  Serial.print(" cm | Gerbang: ");
  Serial.print(gateOpen ? "BUKA" : "TUTUP");
  Serial.print(" | Total: ");
  Serial.println(totalDetections);

  if (distance > 0 && distance < 15) {
    lastDetectedTime = currentTime;

    if (!detecting) {
      detecting = true;
      firstDetectedTime = currentTime;
      alertUsed = false;
      totalDetections++;
      Serial.println(">>> MOBIL TERDETEKSI! <<<");
    }

    if (!alertUsed && !alertActive && (currentTime - firstDetectedTime >= 500)) {
      digitalWrite(ledPin, HIGH);
      digitalWrite(buzzerPin, HIGH);
      alertStartTime = currentTime;
      alertActive = true;
      alertUsed = true;
      Serial.println(">>> ALERT: LED & Buzzer ON");
    }

    if (alertActive && (currentTime - alertStartTime >= 200)) {
      digitalWrite(ledPin, LOW);
      digitalWrite(buzzerPin, LOW);
      alertActive = false;
      Serial.println(">>> ALERT: LED & Buzzer OFF");
    }

    if (!gateOpen && (currentTime - firstDetectedTime >= 2000)) {
      gerbang.write(90);
      gateOpen = true;
      Serial.println(">>> GERBANG DIBUKA");
    }
  }

  if (gateOpen && (currentTime - lastDetectedTime >= 2000)) {
    unsigned long responseTime = currentTime - firstDetectedTime;
    totalResponseTime += responseTime;
    responseCount++;
    
    gerbang.write(0);
    gateOpen = false;
    detecting = false;
    alertUsed = false;
    alertActive = false;

    digitalWrite(ledPin, LOW);
    digitalWrite(buzzerPin, LOW);
    
    Serial.println(">>> GERBANG DITUTUP");
    Serial.print(">>> Waktu respon: ");
    Serial.print(responseTime / 1000.0);
    Serial.println(" detik");
  }

  delay(50);
}
