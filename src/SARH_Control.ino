#include <esp_now.h>
#include <esp_wifi.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include "DASHBOARD.h"
#include <DHT.h>
#include <EEPROM.h>

// =============================================================================
// CONSTANTS AND CONFIGURATION
// =============================================================================
#define EEPROM_SIZE 512
#define CHANNEL 1 
#define ON_BOARD_LED 2

// Network Configuration
const char* WIFI_SSID = "ESP32_WS";
const char* WIFI_PASSWORD = "helloesp32WS";
const IPAddress LOCAL_IP(192, 168, 1, 1);
const IPAddress GATEWAY(192, 168, 1, 1);
const IPAddress SUBNET(255, 255, 255, 0);

// Sensor Pin Definitions
#define PH_PIN 32
#define TURBIDITY_PIN 34
#define TDS_PIN 33
#define MQ7_PIN 39
#define MQ135_PIN 36
#define SOIL_MOISTURE_PIN 35
#define DHT_PIN 4
#define LDR_PIN 5
#define LOW_FLOAT_PIN 25
#define HIGH_FLOAT_PIN 26
#define DHT_TYPE DHT11

// Actuator Pin Definitions
#define FILTER_RELAY_PIN 15
#define LED_RELAY_PIN 21
#define PLANT_RELAY_PIN 23
#define REFILTER_RELAY_PIN 27
#define INLET_FAN_PIN 18
#define OUTLET_FAN_PIN 19

// Sensor Calibration Constants
const float PH_CALIBRATION = 20.4;
const int TURBIDITY_SAMPLES = 600;
const float TDS_EC_CALIBRATION = 1.0;
const float TDS_OFFSET = 0.14;
const float MQ135_RLOAD = 10000;
const float MQ135_RZERO = 28000;
const float MQ7_R0 = 0.236;

// Control Thresholds
const int SOIL_MOISTURE_LOW = 35;
const int SOIL_MOISTURE_HIGH = 75;
const float PH_THRESHOLD = 6.0;
const int TDS_THRESHOLD = 1600;
const float TURBIDITY_THRESHOLD = 35.0;
const float CO_DANGER = 5.0;
const float CO_CAUTION = 3.0;
const float CO_MODERATE = 2.0;
const float CO2_DANGER = 1500.0;
const float CO2_CAUTION = 1000.0;
const float CO2_MODERATE = 800.0;

// =============================================================================
// GLOBAL VARIABLES
// =============================================================================
DHT dht(DHT_PIN, DHT_TYPE);
AsyncWebServer server(80);

// System State Structure
struct SensorData {
  float ph;
  float turbidity;
  unsigned int tds;
  float co2;
  float co;
  int soilMoisture;
  float airMoisture;
  float waterUsed;
  float flowRate;
  bool pump1;
  bool pump2;
  bool refilterPump;
  bool ledStrip;
  bool ventilation;
  bool tankEmpty;
};

SensorData sensorData;

// Control Variables
bool autoMode = true;
bool filterPumpActive = false;
bool plantPumpActive = false;
bool refilterPumpActive = false;
bool ledStripActive = false;
bool waterExist = true;
bool waterPumped = false;
bool doneRefilter = true;
unsigned long previousMillis = 0;
unsigned long startTime = 0;
unsigned long start = 0;
unsigned long current = 0;
const long BLINK_INTERVAL = 500;

// EEPROM Variables
const int EEPROM_ADDR = 0;
float waterUsedValue = 0.0;

// =============================================================================
// SENSOR READING FUNCTIONS (NON-BLOCKING)
// =============================================================================

// Non-blocking sensor state variables
struct SensorState {
  int phBuffer[10];
  int phIndex = 0;
  bool phReady = false;
  unsigned long phLastRead = 0;
  
  long turbiditySum = 0;
  int turbidityCount = 0;
  bool turbidityReady = false;
  unsigned long turbidityLastRead = 0;
  
  unsigned long lastSensorCycle = 0;
  int currentSensor = 0;
};

SensorState sensorState;

float readPHLevel() {
  const unsigned long PH_READ_INTERVAL = 30; // ms between readings
  
  if (millis() - sensorState.phLastRead >= PH_READ_INTERVAL) {
    if (sensorState.phIndex < 10) {
      sensorState.phBuffer[sensorState.phIndex] = analogRead(PH_PIN);
      sensorState.phIndex++;
      sensorState.phLastRead = millis();
      
      if (sensorState.phIndex >= 10) {
        sensorState.phReady = true;
      }
    }
  }
  
  if (sensorState.phReady) {
    // Sort readings (bubble sort)
    for (int i = 0; i < 9; i++) {
      for (int j = i + 1; j < 10; j++) {
        if (sensorState.phBuffer[i] > sensorState.phBuffer[j]) {
          int temp = sensorState.phBuffer[i];
          sensorState.phBuffer[i] = sensorState.phBuffer[j];
          sensorState.phBuffer[j] = temp;
        }
      }
    }
    
    // Average middle 6 values
    unsigned long avgValue = 0;
    for (int i = 2; i < 8; i++) {
      avgValue += sensorState.phBuffer[i];
    }
    
    float voltage = (float)avgValue * 3.3 / 4095.0 / 6.0;
    float phValue = -5.70 * voltage + PH_CALIBRATION;
    
    // Reset for next reading cycle
    sensorState.phIndex = 0;
    sensorState.phReady = false;
    
    Serial.printf("pH: %.2f\n", phValue);
    return phValue;
  }
  
  return sensorData.ph; // Return last known value
}

float readTurbidity() {
  const unsigned long TURBIDITY_READ_INTERVAL = 2; // ms between readings
  const int TURBIDITY_BATCH_SIZE = 50; // Read in smaller batches
  
  if (millis() - sensorState.turbidityLastRead >= TURBIDITY_READ_INTERVAL) {
    if (sensorState.turbidityCount < TURBIDITY_SAMPLES) {
      // Read in small batches to avoid blocking
      int batchEnd = min(sensorState.turbidityCount + TURBIDITY_BATCH_SIZE, TURBIDITY_SAMPLES);
      
      for (int i = sensorState.turbidityCount; i < batchEnd; i++) {
        sensorState.turbiditySum += analogRead(TURBIDITY_PIN);
        yield(); // Allow other tasks to run
      }
      
      sensorState.turbidityCount = batchEnd;
      sensorState.turbidityLastRead = millis();
      
      if (sensorState.turbidityCount >= TURBIDITY_SAMPLES) {
        sensorState.turbidityReady = true;
      }
    }
  }
  
  if (sensorState.turbidityReady) {
    int avgReading = sensorState.turbiditySum / TURBIDITY_SAMPLES;
    float voltage = avgReading * (3.3 / 4095.0);
    float scaledVoltage = voltage * (5.0 / 3.3);
    
    // Map voltage to turbidity (NTU)
    float turbidity = map(scaledVoltage * 100, 20, 281, 1000, 3) / 100.0 * 40;
    
    // Reset for next reading cycle
    sensorState.turbiditySum = 0;
    sensorState.turbidityCount = 0;
    sensorState.turbidityReady = false;
    
    Serial.printf("Turbidity: %.2f NTU\n", turbidity);
    return turbidity;
  }
  
  return sensorData.turbidity; // Return last known value
}

unsigned int readTDS() {
  int rawValue = analogRead(TDS_PIN);
  float voltage = rawValue * 3.3 / 4095.0;
  
  float ec = (voltage * TDS_EC_CALIBRATION) - TDS_OFFSET;
  if (ec < 0) ec = 0;
  
  unsigned int tds = (133.42 * pow(ec, 3) - 255.86 * ec * ec + 857.39 * ec) * 0.75;
  
  Serial.printf("TDS: %d ppm\n", tds);
  return tds;
}

float readCO2() {
  int rawValue = analogRead(MQ135_PIN);
  float voltage = (rawValue / 4095.0) * 3.3;
  float rs = ((3.3 * MQ135_RLOAD) / voltage) - MQ135_RLOAD;
  float ratio = rs / MQ135_RZERO;
  float ppm = 116.6020682 * pow(ratio, -2.769034857) * 0.15625 + 50;
  
  Serial.printf("CO2: %.2f ppm\n", ppm);
  return ppm;
}

float readCO() {
  int rawValue = analogRead(MQ7_PIN);
  float voltage = rawValue * (3.3 / 4095.0);
  float rs = (3.3 - voltage) / voltage;
  float ratio = rs / MQ7_R0;
  float ppm = 10.0 * pow(ratio, -2.0) * 3.0;
  
  Serial.printf("CO: %.2f ppm\n", ppm);
  return ppm;
}

int readSoilMoisture() {
  int rawValue = analogRead(SOIL_MOISTURE_PIN);
  int moisture = map(rawValue, 4095, 1500, 0, 100);
  moisture = constrain(moisture, 0, 100);
  
  Serial.printf("Soil Moisture: %d%%\n", moisture);
  return moisture;
}

bool readTankStatus() {
  bool lowFloat = digitalRead(LOW_FLOAT_PIN);
  bool highFloat = digitalRead(HIGH_FLOAT_PIN);
  return (lowFloat == LOW && highFloat == LOW);
}

// =============================================================================
// ACTUATOR CONTROL FUNCTIONS
// =============================================================================
void setFilterPump(bool state) {
  digitalWrite(FILTER_RELAY_PIN, state ? HIGH : LOW);
  sensorData.pump1 = state;
  Serial.printf("Filter pump: %s\n", state ? "ON" : "OFF");
}

void setPlantPump(bool state) {
  digitalWrite(PLANT_RELAY_PIN, state ? HIGH : LOW);
  sensorData.pump2 = state;
  
  if (state) {
    startTime = millis();
    Serial.println("Plant pump: ON");
  } else {
    Serial.println("Plant pump: OFF");
  }
}

void setRefilterPump(bool state) {
  digitalWrite(REFILTER_RELAY_PIN, state ? LOW : HIGH); // Inverted logic
  sensorData.refilterPump = state;
  Serial.printf("Refilter pump: %s\n", state ? "ON" : "OFF");
}

void setLEDStrip(bool state) {
  digitalWrite(LED_RELAY_PIN, state ? LOW : HIGH); // Inverted logic
  sensorData.ledStrip = state;
  Serial.printf("LED Strip: %s\n", state ? "ON" : "OFF");
}

void setFanSpeed(int speedPercent) {
  int pwmValue = map(constrain(speedPercent, 30, 100), 30, 100, 100, 255);
  analogWrite(INLET_FAN_PIN, pwmValue);
  analogWrite(OUTLET_FAN_PIN, pwmValue);
  sensorData.ventilation = (speedPercent > 30);
  Serial.printf("Fan speed: %d%% (PWM: %d)\n", speedPercent, pwmValue);
}

int calculateFanSpeed(float co, float co2) {
  if (co > CO_DANGER || co2 > CO2_DANGER) {
    return 100; // Full speed
  } else if (co > CO_CAUTION || co2 > CO2_CAUTION) {
    return 70;  // Medium speed
  } else if (co > CO_MODERATE || co2 > CO2_MODERATE) {
    return 50;  // Low speed
  }
  return 30;    // Minimum speed
}

void updateWaterUsage() {
  if (waterPumped) {
    unsigned long elapsedTime = millis() - startTime;
    float flowRate = random(135, 156) / 100.0; // 1.35 to 1.55 L/min
    float waterUsed = (flowRate / 60000.0) * elapsedTime; // Convert to L
    
    waterUsedValue += waterUsed;
    EEPROM.put(EEPROM_ADDR, waterUsedValue);
    EEPROM.commit();
    
    sensorData.waterUsed = waterUsedValue;
    waterPumped = false;
    
    Serial.printf("Water used this session: %.3f L, Total: %.3f L\n", waterUsed, waterUsedValue);
  }
}

// =============================================================================
// AUTO MODE CONTROL LOGIC
// =============================================================================
void handleAutoMode() {  
  if(doneRefilter){
    // Water level control
    bool LOW_TANK = digitalRead(LOW_FLOAT_PIN);
    bool HIGH_TANK = digitalRead(HIGH_FLOAT_PIN);
    
    if (!LOW_TANK && !HIGH_TANK) {
      setFilterPump(true);
      waterExist = false;
    } 
    if (LOW_TANK && HIGH_TANK) {
      setFilterPump(false);
      waterExist = true;
    }
    
    if (waterExist) {
      Serial.println("Water available");
      
      // Water quality check
      bool needsRefiltering = (sensorData.ph < PH_THRESHOLD || sensorData.tds > TDS_THRESHOLD || sensorData.turbidity > TURBIDITY_THRESHOLD);
      setRefilterPump(needsRefiltering);
      
      if (!needsRefiltering) {
        // Soil moisture control
        if (sensorData.soilMoisture < SOIL_MOISTURE_LOW) {
          setPlantPump(true);
          waterPumped = true;
        } else if (sensorData.soilMoisture > SOIL_MOISTURE_HIGH) {
          setPlantPump(false);
          updateWaterUsage();
        }
      } else {
        digitalWrite(REFILTER_RELAY_PIN, LOW);
        delay(30000);
        digitalWrite(REFILTER_RELAY_PIN, HIGH);
        start = millis();
        doneRefilter = false;
      }
    }
  }
  // Air quality control
  int fanSpeed = calculateFanSpeed(sensorData.co, sensorData.co2);
  setFanSpeed(fanSpeed);
  
  // Light control
  bool isDark = (digitalRead(LDR_PIN) == HIGH);
  setLEDStrip(isDark);
}

// =============================================================================
// WEB SERVER SETUP
// =============================================================================
void setupWebServer() {
  // Main dashboard
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", htmlPage);
  });
  
  // Sensor data API
  server.on("/api/sensors", HTTP_GET, [](AsyncWebServerRequest *request) {
    StaticJsonDocument<500> doc;
    doc["ph"] = sensorData.ph;
    doc["turbidity"] = sensorData.turbidity;
    doc["tds"] = sensorData.tds;
    doc["co2"] = sensorData.co2;
    doc["co"] = sensorData.co;
    doc["soilMoisture"] = sensorData.soilMoisture;
    doc["airMoisture"] = sensorData.airMoisture;
    doc["waterUsed"] = sensorData.waterUsed;
    doc["flowRate"] = sensorData.flowRate;
    doc["pump1"] = sensorData.pump1;
    doc["pump2"] = sensorData.pump2;
    doc["refilterPump"] = sensorData.refilterPump;
    doc["ledStrip"] = sensorData.ledStrip;
    doc["ventilation"] = sensorData.ventilation;
    doc["tankEmpty"] = sensorData.tankEmpty;
    
    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
  });
  
  // Mode control
  server.on("/api/mode", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, 
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      String body = String((char*)data);
      StaticJsonDocument<200> doc;
      
      if (deserializeJson(doc, body) == DeserializationError::Ok && doc.containsKey("auto")) {
        autoMode = doc["auto"];
        previousMillis = millis();
        
        // Reset all actuators when switching modes
        setFilterPump(false);
        setPlantPump(false);
        setRefilterPump(false);
        setLEDStrip(false);
        setFanSpeed(30);
        
        Serial.printf("Mode changed to: %s\n", autoMode ? "Auto" : "Manual");
        request->send(200, "application/json", 
          "{\"status\":\"success\", \"autoMode\":" + String(autoMode ? "true" : "false") + "}");
      } else {
        request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
      }
    });
  
  // Manual control endpoints
  setupManualControls();
}

void setupManualControls() {
  // Filter pump controls
  server.on("/api/filter-pump/press", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setFilterPump(true);
      digitalWrite(ON_BOARD_LED, HIGH);
      request->send(200, "application/json", "{\"status\":\"success\", \"pump1\":true}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  server.on("/api/filter-pump/release", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setFilterPump(false);
      digitalWrite(ON_BOARD_LED, LOW);
      request->send(200, "application/json", "{\"status\":\"success\", \"pump1\":false}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  // Plant pump controls
  server.on("/api/plant-pump/press", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setPlantPump(true);
      waterPumped = true;
      digitalWrite(ON_BOARD_LED, HIGH);
      request->send(200, "application/json", "{\"status\":\"success\", \"pump2\":true}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  server.on("/api/plant-pump/release", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setPlantPump(false);
      updateWaterUsage();
      digitalWrite(ON_BOARD_LED, LOW);
      request->send(200, "application/json", "{\"status\":\"success\", \"pump2\":false}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  // Refilter pump controls
  server.on("/api/refilter-pump/press", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setRefilterPump(true);
      digitalWrite(ON_BOARD_LED, HIGH);
      request->send(200, "application/json", "{\"status\":\"success\", \"refilterPump\":true}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  server.on("/api/refilter-pump/release", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      setRefilterPump(false);
      digitalWrite(ON_BOARD_LED, LOW);
      request->send(200, "application/json", "{\"status\":\"success\", \"refilterPump\":false}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
  
  // LED strip control
  server.on("/api/led-strip/toggle", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!autoMode) {
        String body = String((char*)data);
        StaticJsonDocument<200> doc;
        
        if (deserializeJson(doc, body) == DeserializationError::Ok && doc.containsKey("state")) {
          bool state = doc["state"];
          setLEDStrip(state);
          digitalWrite(ON_BOARD_LED, state ? HIGH : LOW);
          request->send(200, "application/json", 
            "{\"status\":\"success\", \"ledStrip\":" + String(state ? "true" : "false") + "}");
        } else {
          request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
        }
      } else {
        request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
      }
    });
  
  // Fan speed control
  server.on("/api/fan-speed", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
      if (!autoMode) {
        String body = String((char*)data);
        StaticJsonDocument<200> doc;
        
        if (deserializeJson(doc, body) == DeserializationError::Ok && doc.containsKey("speed")) {
          int speed = doc["speed"];
          setFanSpeed(speed);
          request->send(200, "application/json", 
            "{\"status\":\"success\", \"fanSpeed\":" + String(speed) + "}");
        } else {
          request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Invalid JSON\"}");
        }
      } else {
        request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
      }
    });
  
  // Water usage reset
  server.on("/api/water/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    if (!autoMode) {
      waterUsedValue = 0.0;
      EEPROM.put(EEPROM_ADDR, waterUsedValue);
      EEPROM.commit();
      sensorData.waterUsed = waterUsedValue;
      Serial.println("Water usage counter reset");
      request->send(200, "application/json", "{\"status\":\"success\"}");
    } else {
      request->send(400, "application/json", "{\"status\":\"error\", \"message\":\"Auto mode active\"}");
    }
  });
}

// =============================================================================
// INITIALIZATION FUNCTIONS
// =============================================================================
void initializePins() {
  // Output pins
  pinMode(ON_BOARD_LED, OUTPUT);
  pinMode(FILTER_RELAY_PIN, OUTPUT);
  pinMode(REFILTER_RELAY_PIN, OUTPUT);
  pinMode(PLANT_RELAY_PIN, OUTPUT);
  pinMode(INLET_FAN_PIN, OUTPUT);
  pinMode(OUTLET_FAN_PIN, OUTPUT);
  pinMode(LED_RELAY_PIN, OUTPUT);
  
  // Input pins
  pinMode(LDR_PIN, INPUT);
  pinMode(LOW_FLOAT_PIN, INPUT_PULLUP);
  pinMode(HIGH_FLOAT_PIN, INPUT_PULLUP);
  
  // Initialize relay states (all OFF)
  digitalWrite(LED_RELAY_PIN, HIGH);    // Inverted logic
  digitalWrite(ON_BOARD_LED, LOW);
  digitalWrite(FILTER_RELAY_PIN, LOW);
  digitalWrite(REFILTER_RELAY_PIN, HIGH); // Inverted logic
  digitalWrite(PLANT_RELAY_PIN, LOW);
  
  // Initialize fans at low speed
  analogWrite(OUTLET_FAN_PIN, 77);
  analogWrite(INLET_FAN_PIN, 77);
}

void initializeWiFi() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
  delay(1000);
  WiFi.softAPConfig(LOCAL_IP, GATEWAY, SUBNET);
  
  Serial.print("WiFi AP started. IP address: ");
  Serial.println(WiFi.softAPIP());
}

void initializeSensorData() {
  sensorData = {
    .ph = 7.0,
    .turbidity = 2.5,
    .tds = 450,
    .co2 = 400,
    .co = 0.5,
    .soilMoisture = 45,
    .airMoisture = 50,
    .waterUsed = 0,
    .flowRate = 1.4,
    .pump1 = false,
    .pump2 = false,
    .refilterPump = false,
    .ledStrip = false,
    .ventilation = false,
    .tankEmpty = false
  };
  
  // Load water usage from EEPROM
  EEPROM.get(EEPROM_ADDR, waterUsedValue);
  sensorData.waterUsed = waterUsedValue;
}

// =============================================================================
// MAIN FUNCTIONS
// =============================================================================
void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  
  // Initialize EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    return;
  }
  
  // Initialize sensor state
  memset(&sensorState, 0, sizeof(sensorState));
  
  // Initialize components
  initializePins();
  initializeWiFi();
  initializeSensorData();
  
  // Initialize DHT sensor
  dht.begin();
  
  // Setup web server
  setupWebServer();
  server.begin();
  
  // Shorter sensor warm-up period
  Serial.println("Warming up sensors...");
  delay(5000); // Reduced from 10000
  Serial.println("System ready!");
}

void loop() {
  // Feed the watchdog
  yield();
  
  bool tankEmpty = readTankStatus();
  sensorData.tankEmpty = tankEmpty;

  // Non-blocking sensor reading cycle (rotate through sensors)
  const unsigned long SENSOR_CYCLE_INTERVAL = 100; // ms
  
  if (millis() - sensorState.lastSensorCycle >= SENSOR_CYCLE_INTERVAL) {
    switch (sensorState.currentSensor) {
      case 0:
        sensorData.ph = readPHLevel();
        break;
      case 1:
        sensorData.turbidity = readTurbidity();
        break;
      case 2:
        sensorData.tds = readTDS();
        break;
      case 3:
        sensorData.co2 = readCO2();
        break;
      case 4:
        sensorData.co = readCO();
        break;
      case 5:
        sensorData.soilMoisture = readSoilMoisture();
        break;
      case 6:
        sensorData.airMoisture = dht.readHumidity();
        break;
    }
    
    sensorState.currentSensor = (sensorState.currentSensor + 1) % 7;
    sensorState.lastSensorCycle = millis();
  }
  
  // Handle control logic
  if (autoMode) {
    handleAutoMode();
  }
  
  // Feed watchdog again
  yield();
  
  // Minimal delay to prevent tight looping
  delay(10);
  
  if(!doneRefilter){
    current = millis() - start;
    if(current > 60000){
      doneRefilter = true;
    }
  } else{
    current = 0;
  }
}