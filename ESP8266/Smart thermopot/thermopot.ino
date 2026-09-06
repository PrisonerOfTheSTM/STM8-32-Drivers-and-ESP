#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <HX711.h>

// НАСТРОЙКИ
#define ONE_WIRE_BUS 2
#define LOADCELL_DT_PIN 12
#define LOADCELL_SCK_PIN 13
#define CALIBRATION_FACTOR -471.5f
#define VALVE_FILL_PIN 5 // D1
#define HEATER_PIN 16 // D0

const char* apSSID = "ESP8266_Sensor";
const char* apPassword = "";
const char* hostname = "esp8266";

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
HX711 scale;
ESP8266WebServer server(80);
DNSServer dnsServer;

float currentTemp = 0;
float currentWeight = 0;
float currentVolume = 0;

String tempError = "";
String weightError = "";

bool wifiConnected = false;

unsigned long wifiConnectStart = 0;

int hx711FailCount = 0;

// Переменные для контроля заполнения
float targetVolume = 0;
float targetTemp = 0;

bool isFilling = false;
bool isHeating = false;
bool emergencyStop = false;

struct Config {
  char ssid[32];
  char password[32];
  bool isValid;
};

Config config;

// ПЛОТНОСТИ
struct DensityPoint {
  float temp;
  float density;
};

const DensityPoint densityTable[] = {
  {20.0, 998.2},
  {25.0, 997.05},
  {50.0, 988.0},
  {80.0, 971.8}
};
const int densityTableSize = sizeof(densityTable) / sizeof(densityTable[0]);

//ПРОТОТИПЫ ФУНКЦИЙ
void loadConfig();
void saveConfig();
void readSensors();
void startConfigMode();
void setupConfigServer();
void setupWebServer();
void handleConfigRoot();
void handleConnect();
void handleMainPage();
void handleData();
void handleReset();
void handleSetTarget();
void handleEmergencyStop();
float getDensityAtTemp(float temp);
float calculateVolume(float weight, float density);
void checkFillingProcess();
void emergencyShutdown();
void startFilling();
void stopHeating();

// SETUP
void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.println("\n\nЗапуск ESP8266...");
  
  // Настройка пинов управления
  pinMode(VALVE_FILL_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  digitalWrite(VALVE_FILL_PIN, LOW);
  digitalWrite(HEATER_PIN, LOW);
  
  EEPROM.begin(512);
  loadConfig();
  
  sensors.begin();
  if (sensors.getDeviceCount() == 0) {
    tempError = "Датчик не найден";
  }
  
  scale.begin(LOADCELL_DT_PIN,  LOADCELL_SCK_PIN);
  scale.set_scale(CALIBRATION_FACTOR);
  scale.tare();
  
  if (config.isValid) {
    WiFi.mode(WIFI_STA);
    WiFi.hostname(hostname);
    WiFi.begin(config.ssid, config.password);
    wifiConnectStart = millis();
    Serial.print("Подключение к WiFi");
    while (WiFi.status() != WL_CONNECTED && millis() - wifiConnectStart < 30000) {
      delay(500);
      Serial.print(".");
  }
    
    if (WiFi.status() == WL_CONNECTED) {
      wifiConnected = true;
    if (MDNS.begin(hostname)) {
      Serial.println("\n mDNS запущен!");
    }
    setupWebServer();
    Serial.print("IP адрес: ");
    Serial.println(WiFi.localIP());
    } else {
      startConfigMode();
    }
  } else {
    startConfigMode();
  }
}

// LOOP
void loop() {
  server.handleClient();
  MDNS.update();
  if (!wifiConnected) {
  dnsServer.processNextRequest();
  }
  
  static unsigned long lastRead = 0;
  
  if (millis() - lastRead > 1000) {
    lastRead = millis();
    readSensors();
    
    // Проверка процесса заполнения и нагрева
    if (!emergencyStop) {
      if (isFilling || isHeating) {
        checkFillingProcess();
      }
    }
  }
}

// ФУНКЦИИ ДАТЧИКОВ
void readSensors() {
  
  sensors.requestTemperatures();
  float temp = sensors.getTempCByIndex(0);
  
  if (temp != DEVICE_DISCONNECTED_C) {
    currentTemp = temp;
    tempError = "";
  } 
  else {
    tempError = "Датчик не найден";
  }
  
  if (scale.is_ready()) {
    float weight = scale.get_units(5);
    
    if (weight < 0) weight = 0;
    
    currentWeight = weight;
    hx711FailCount = 0;
    weightError = "";
    
    // Расчет объема на основе текущей температуры
    float density = getDensityAtTemp(currentTemp);
    currentVolume = calculateVolume(currentWeight, density);
  } 
  else {
    hx711FailCount++;
    if (hx711FailCount >= 3) {
      weightError = "HX711 не отвечает";
    } 
    else {
      weightError = "";
    }
  }
}
// ФУНКЦИИ ДЛЯ РАСЧЕТА ПЛОТНОСТИ И ОБЪЕМА

float getDensityAtTemp(float temp) {
  if (temp <= densityTable[0].temp) {
    return densityTable[0].density;
  }
  
  if (temp >= densityTable[densityTableSize -  1].temp) {
    return densityTable[densityTableSize -  1].density;
  }
  
  for (int i = 0; i < densityTableSize - 1; i++) {
    if (temp >= densityTable[i].temp && temp <= densityTable[i + 1].temp) {
      float t1 = densityTable[i].temp;
      float d1 = densityTable[i].density;
      float t2 = densityTable[i + 1].temp;
      float d2 = densityTable[i + 1].density;
      float density = d1 + (d2 - d1) * (temp - t1) /
      (t2 - t1);
      return density;
    }
  }
  return 997.0;
}

float calculateVolume(float weight, float density) {
  if (density <= 0) return 0; // Переводим в литры
  return weight / density;
}

// ФУНКЦИИ УПРАВЛЕНИЯ
void startFilling() {
  if (!emergencyStop) {
    digitalWrite(VALVE_FILL_PIN, HIGH);
    isFilling = true;
    Serial.println("Клапан заполнения открыт");
    
    if (targetTemp > currentTemp) {
      digitalWrite(HEATER_PIN, HIGH); //
      Включаем нагреватель
      isHeating = true;
      Serial.println("Нагреватель включен");
    }
  }
}

void emergencyShutdown() {
  digitalWrite(VALVE_FILL_PIN, LOW); //  Закрываем клапан
  digitalWrite(HEATER_PIN, LOW); //  Выключаем нагреватель
  isFilling = false;
  isHeating = false;
  emergencyStop = true;
  Serial.println("Экстренная остановка! D1 и  D0 сброшены в 0");
}

void stopHeating() {
  digitalWrite(HEATER_PIN, LOW); //  Выключаем нагреватель
  isHeating = false;
  Serial.println("Нагреватель выключен");
}

void checkFillingProcess() { // Проверяем температуру
  if (isHeating && currentTemp >= targetTemp)
  {
    stopHeating();
    Serial.println("Достигнута целевая температура");
  }
  if (isFilling && currentVolume >= targetVolume) {
    digitalWrite(VALVE_FILL_PIN, LOW); // Закрываем клапан
    isFilling = false;
    Serial.println("Достигнут целевой объем, клапан закрыт");
    if (currentTemp < targetTemp && !isHeating)
    {
      digitalWrite(HEATER_PIN, HIGH);
      isHeating = true;
    }
  }
}

// РЕЖИМ НАСТРОЙКИ
void startConfigMode() {
  wifiConnected = false;
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID, apPassword);
  dnsServer.start(53, "*", WiFi.softAPIP());
  setupConfigServer();
  Serial.println("\n РЕЖИМ НАСТРОЙКИ ");
}

void setupConfigServer() {
  server.on("/", handleConfigRoot);
  server.on("/connect", handleConnect);
  server.on("/data", handleData);
  server.begin();
}

void setupWebServer() {
  server.on("/", handleMainPage);
  server.on("/data", handleData);
  server.on("/reset", handleReset);
  server.on("/setTarget", handleSetTarget);
  server.on("/emergencyStop",
  handleEmergencyStop);
  server.begin();
}


void handleConfigRoot() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <meta charset="UTF-8">
        <meta name="viewport"
        content="width=device-width, initial-scale=1.0">
        
        <title>Настройка WiFi</title>
        
        <style>
          body {
            font-family: Arial, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
          }
          
          .container {
            background: white;
            border-radius: 20px;
            padding: 40px;
            max-width: 400px;
            width: 100%;
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
          }
          
          h2 { color: #333; text-align: center; }
          
          input {
            width: 100%;
            padding: 12px;
            margin: 10px 0;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 16px;
          }
          
          button {
            width: 100%;
            padding: 12px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 18px;
            cursor: pointer;
            margin-top: 20px;
          }
          
          .info {
            text-align: center;
            color: #666;
            font-size: 12px;
            margin-top: 20px;
          }
        </style>
      </head>
      
      <body>
        <div class="container">
          <h2> Настройка WiFi</h2>
          <form action="/connect" method="POST">
            <input type="text" name="ssid"
            placeholder="Название Wi-Fi (SSID)" required>
            <input type="password" name="password" placeholder="Пароль">
            <button type="submit">Сохранить и подключиться</button>
          </form>
          <div class="info">
          После сохранения ESP перезагрузится<br>
          Открывайте <b>http://esp8266.local</b>
          </div>
        </div>
      </body>
    </html>
  )rawliteral";
  
  server.send(200, "text/html", html);
}

void handleConnect() {
  if (server.hasArg("ssid")) {
    server.arg("ssid").toCharArray(config.ssid,
    sizeof(config.ssid));
    server.arg("password").toCharArray(config.password, sizeof(config.password));
    config.isValid = true;
    saveConfig();
    
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html>
        <head>
          <meta charset="UTF-8">
          <meta http-equiv="refresh"
          content="10;url=http://esp8266.local">
    
          <style>
            body {
              font-family: Arial, sans-serif;
              background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
              display: flex;
              justify-content: center;
              align-items: center;
              height: 100vh;
            }
    
            .container {
              background: white;
              padding: 40px;
              border-radius: 20px;
              text-align: center;
            }
            .url {
              background: #f0f0f0;
              padding: 10px;
              border-radius: 10px;
              font-family: monospace;
              font-size: 18px;
              margin: 20px 0;
            }
          </style>
        </head>
    
        <body>
          <div class="container">
          <h2>✅ Настройки сохранены!</h2>
          <p>ESP перезагружается...</p>
          <p>Через 10 секунд откроется страница:</p>
          <div class="url">http://esp8266.local</div>
          </div>
        </body>
      </html>
    )rawliteral";
    
    server.send(200, "text/html", html);
    delay(1000);
    
    ESP.restart();
  }
}

void handleMainPage() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html>
      <head>
        <meta charset="UTF-8">
        <meta name="viewport"
        content="width=device-width, initial-scale=1.0">
    
        <title>Управление нагревом ESP8266</title>
    
        <style>
          body {
            font-family: 'Segoe UI', sans-serif;
            background: linear-gradient(135deg,
            #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            justify-content: center;
            align-items: center;
            padding: 20px;
          }
      
          .container {
            background: white;
            border-radius: 30px;
            padding: 40px;
            text-align: center;
            max-width: 500px;
            width: 100%;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
          }
      
          h1 { color: #333; margin-bottom: 30px; }
      
          .card {
            border-radius: 20px;
            padding: 30px;
            margin: 20px 0;
            color: white;
          }
      
          .card-temp { background: lineargradient(135deg, #f093fb 0%, #f5576c 100%); }
          .card-weight { background: lineargradient(135deg, #4facfe 0%, #00f2fe 100%); }
          .card-volume { background: lineargradient(135deg, #43e97b 0%, #38f9d7 100%); }
      
          .value { font-size: 64px; font-weight: bold; }
          .unit { font-size: 24px; margin-left: 10px; }
          .label { font-size: 20px; opacity: 0.9; }
          .error { font-size: 14px; margin-top: 10px; opacity: 0.9; }
      
          .control-section {
            margin-top: 30px;
            padding: 20px;
            background: #f8f9fa;
            border-radius: 15px;
          }
      
          .input-group {
            margin: 10px 0;
          }
      
          input[type="number"] {
            width: 100%;
            padding: 10px;
            border: 2px solid #ddd;
            border-radius: 10px;
            font-size: 16px;
            margin: 5px 0;
          }
      
          .btn {
            width: 100%;
            padding: 12px;
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 18px;
            cursor: pointer;
            margin: 5px 0;
          }
      
          .btn-start { background: lineargradient(135deg, #43e97b 0%, #38f9d7 100%); }
          .btn-stop { background: #f5576c; }
          .btn-reset { background: #764ba2; }
          .btn:hover { opacity: 0.8; }
          .status {
            font-size: 16px;
            margin: 10px 0;
            padding: 10px;
            background: #e9ecef;
            border-radius: 10px;
          }
        </style>
      </head>
  
      <body>
        <div class="container">
        <h1> Мониторинг и управление</h1>
  
        <div class="card card-temp">
        <div class="label"> Температура</div>
        <div class="value">
          <span id="tempValue">--.-</span>
          <span class="unit">°C</span>
        </div>
        <div class="error"
          id="tempError"></div>
        </div>
        <div class="card card-weight">
        <div class="label"> Масса</div>
        <div class="value">
        <span id="weightValue">---</span>
        <span class="unit">г</span>
        </div>
        <div class="error" id="weightError"></div>
        </div>
        <div class="card card-volume">
        <div class="label"> Объем</div>
        <div class="value">
        <span id="volumeValue">---</span>
        <span class="unit">л</span>
        </div>
        <div class="error"
        id="volumeError"></div>
        </div>
        <div class="control-section">
        <h3> Управление заполнением и
        нагревом</h3>
        <div class="input-group">
        <label>Целевой объем (2.0 - 3.0 л):</label>
        <input type="number" id="targetVolume" min="2.0" max="3.0" step="0.1" value="2.5">
        </div>
        <div class="input-group">
        <label>Целевая температура (20 - 80 °C):</label>
        <input type="number" id="targetTemp" min="20" max="80" step="1" value="25">
        </div>
        <div class="status">
        Статус: <span id="statusText">Ожидание</span>
        </div>
        <button class="btn btn-start"
        onclick="startProcess()"> Начать процесс</button>
        <button class="btn btn-stop"
        onclick="emergencyStop()"> Экстренная остановка</button>
        <button class="btn btn-reset" onclick="resetSettings()"> Сбросить настройки WiFi</button>
        </div>
        </div>
        <script>
          function updateData() {
            fetch('/data')
            .then(response => response.json())
            .then(data => {
              if (data.tempError) {
                document.getElementById('temp Value').innerHTML = '--';
                document.getElementById('temp Error').innerHTML = ' ' + data.tempError;
              } 
              else {
                document.getElementById('temp Value').innerHTML = data.temp.toFixed(1);
                document.getElementById('temp Error').innerHTML = '';
              }
              if (data.weightError) {
                document.getElementById('weig
                htError').innerHTML = ' ' + data.weightError;
              } 
              else if (data.weight !== null) {
                document.getElementById('weig
                htValue').innerHTML = data.weight.toFixed(1);
                document.getElementById('weig
                htError').innerHTML = '';
              }
              if (data.volume !== null) {
                document.getElementById('volu
                meValue').innerHTML = data.volume.toFixed(3);
              }
              // Обновляем статус
              let status = 'Ожидание';
              if (data.emergencyStop) {
                status = ' Аварийная остановка';
              } 
              else if (data.isFilling && data.isHeating) {
                status = ' Заполнение и нагрев...';
              } 
              else if (data.isFilling) {
                status = ' Заполнение...';
              } 
              else if (data.isHeating) {
                status = ' Нагрев...';
              }
              document.getElementById('statusText').innerHTML = status;
            })
            .catch(() => {
              document.getElementById('tempError').innerHTML = ' Ошибка соединения';
            });
          }
          function startProcess() {
            const volume = parseFloat(document.getElementById('targetVolume').value);
            const temp = parseFloat(document.getElementById('targetTemp').value);
            if (volume < 2.0 || volume > 3.0) {
              alert('Объем должен быть от 2.0 до 3.0 литров!');
              return;
            }
            if (temp < 20 || temp > 80) {
              alert('Температура должна быть от 20 до 80°C!');
              return;
            }
            fetch('/setTarget?volume=' + volume + '&temp=' + temp)
            .then(response => response.text())
            .then(data => {
              if (data === 'OK') {
                alert('Процесс запущен!');
              } 
              else {
                alert('Ошибка: ' + data);
              }
            });
          }
          function emergencyStop() {
            if (confirm('Вы уверены, что хотите выполнить экстренную остановку? Все пины будут сброшены в 0.')) {
              fetch('/emergencyStop')
              .then(response => response.text())
              .then(data => {
                alert(data);
              });
            }
          }
          function resetSettings() {
            if (confirm('Сбросить все настройки
            WiFi?')) {
              fetch('/reset').then(() => {
                alert('Настройки сброшены!');
                setTimeout(() => location.reload(), 2000);
              });
            }
          }
          updateData();
          setInterval(updateData, 1000);
        </script>
      </body>
    </html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleData() {
  String json = "{";
  
  json += "\"temp\":" + String(currentTemp) + ",";
  json += "\"tempError\":\"" + tempError + "\",";
  json += "\"weight\":" + String(currentWeight) + ",";
  json += "\"weightError\":\"" + weightError + "\",";
  json += "\"volume\":" + String(currentVolume) + ",";
  json += "\"isFilling\":" + String(isFilling ? "true" : "false") + ",";
  json += "\"isHeating\":" + String(isHeating ? "true" : "false") + ",";
  json += "\"emergencyStop\":" + String(emergencyStop ? "true" : "false") + ",";
  json += "\"targetVolume\":" + String(targetVolume) + ",";
  json += "\"targetTemp\":" + String(targetTemp);
  json += "}";
  
  server.send(200, "application/json", json);
}

void handleSetTarget() {
  if (server.hasArg("volume") && server.hasArg("temp")) {
    float volume = server.arg("volume").toFloat();
    float temp = server.arg("temp").toFloat();
    
    // Проверка диапазонов
    if (volume < 2.0 || volume > 3.0) {
      server.send(400, "text/plain", "Объем должен быть от 2.0 до 3.0 литров");
      return;
    }
    
    if (temp < 20 || temp > 80) {
      server.send(400, "text/plain", "Температура должна быть от 20 до 80°C");
      return;
    }
    
    targetVolume = volume;
    targetTemp = temp;
    emergencyStop = false;
    
    startFilling();
    server.send(200, "text/plain", "OK");
  } 
  else {
    server.send(400, "text/plain", "Отсутствуют параметры");
  }
}

void handleEmergencyStop() {
  emergencyShutdown();
  server.send(200, "text/plain", "Экстренная
  остановка! D1 и D0 сброшены в 0");
}

void handleReset() {
  config.isValid = false;
  saveConfig();
  server.send(200, "text/plain", "OK");
  delay(500);
  ESP.restart();
}

// ========== РАБОТА С EEPROM ==========
void saveConfig() {
  EEPROM.put(0, config);
  EEPROM.commit();
  Serial.println("Настройки сохранены в EEPROM");
}

void loadConfig() {
  EEPROM.get(0, config);
  if (config.isValid) {
    Serial.println("Настройки загружены из
    EEPROM");
    Serial.print("SSID: ");
    Serial.println(config.ssid);
  } 
  else {
    Serial.println("EEPROM пуст");
  }
}
