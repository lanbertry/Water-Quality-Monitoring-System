#include <LiquidCrystal_I2C.h>
#include <BlinkControl.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SoftwareSerial.h>

// Sensor Pins
#define temp_pin 2
#define ph_pin A1
#define turb_pin A0
#define buzz 6

// Professional Thresholds
#define TURB_SAFE_MAX 25
#define TURB_WARNING 60
#define TURB_DANGER 80

#define PH_SAFE_MIN 4.5
#define PH_SAFE_MAX 9.0
#define PH_WARNING_LOW 4.5
#define PH_WARNING_HIGH 8.5
#define PH_DANGER_LOW 4
#define PH_DANGER_HIGH 9

#define TEMP_SAFE_MIN 20
#define TEMP_SAFE_MAX 35
#define TEMP_WARNING_LOW 22
#define TEMP_WARNING_HIGH 34
#define TEMP_DANGER_LOW 20
#define TEMP_DANGER_HIGH  

// System Variables
float calibration_value = 24.34 - 0.19;
int buffer_arr[10], temp;
float ph_act, water_temp;
int turbidity, sensorValue;
int saveData = 0;

// Alert System
unsigned long buzzerPreviousMillis = 0;
bool buzzerState = false;
int currentBuzzerInterval = 1000;  // Initial buzzer interval (ms)
String dangerSensor = "";
float dangerProximity = 0.0;

// WiFi Status
bool wifiConnected = false;
unsigned long lastWifiCheck = 0;
const unsigned long wifiCheckInterval = 4000;

// Flashing Variables
unsigned long flashMillis = 0;
bool isFlashing = false;

// Timing
unsigned long prev_millis = 0;
const unsigned long interval = 1000;
String readings;

// Hardware Objects
OneWire oneWire(temp_pin);
DallasTemperature temp_sensor(&oneWire);
SoftwareSerial esp(5, 4);
LiquidCrystal_I2C lcd(0x27, 20, 4);

// WiFi icon (custom character)
byte wifiIcon[8] = {
  B00000,
  B01110,
  B10001,
  B00100,
  B01010,
  B00000,
  B00100,
  B00000
};

byte noWifi[8] = {
  B10000,
  B01110,
  B11001,
  B00100,
  B01010,
  B00010,
  B00101,
  B00000
};

byte dataSaveIcon[8] = {
  B00000, B00100, B00100, B00100, B11111, B01110, B00100, B00000
};

byte warning[8] = {
  B10000,
  B01110,
  B11001,
  B00100,
  B01010,
  B00010,
  B00101,
  B00000
};



void setup() {
  Serial.begin(115200);
  esp.begin(9600);
  Wire.begin();
  
  lcd.begin();
  lcd.backlight();
  lcd.createChar(0, wifiIcon);
  lcd.createChar(1, noWifi); 
  lcd.createChar(2, dataSaveIcon);
  lcd.createChar(3, warning);
  temp_sensor.begin();
  
  pinMode(buzz, OUTPUT);
  pinMode(turb_pin, INPUT);
  pinMode(ph_pin, INPUT);

  // Startup Message
  Serial.println("Welcome");
  lcd.setCursor(5, 1);
  lcd.print("Welcome");
  delay(2000);
  lcd.setCursor(3, 0);
  lcd.print("Water Quality");
  lcd.setCursor(5, 1);
  lcd.print("Monitoring");
  delay(3000);
  lcd.clear();
  lcd.print("Quiambao");
  delay(500);
  lcd.setCursor(8, 1);
  lcd.print("Tandoy");
  delay(500);
  lcd.setCursor(14, 2);
  lcd.print("Tecson");
  delay(3000);
  lcd.clear();
}

void loop() {
  unsigned long currentMillis = millis();
  display_data();

    // Check WiFi status regularly
  if (currentMillis - lastWifiCheck >= wifiCheckInterval) {
    lastWifiCheck = currentMillis;
    checkWifiStatus();
  }
  
  if (currentMillis - prev_millis >= interval) {
    prev_millis = currentMillis;
    
    // Sensor Readings
    ph_readings();
    turb_reading();
    temp_read();
    
    // Data Processing
    checkDangerLevels();

    
    // ESP Communication
    readings = String(ph_act, 2) + "," + String(turbidity) + "," + String(water_temp) + "&";
    esp.print(readings);
    Serial.println(readings);
  }

  // Buzzer Control
  handleBuzzer();
}

void checkWifiStatus() {
    while (esp.available() > 0) {
        String wifiStatus = esp.readStringUntil('&');
        if (wifiStatus.startsWith("WIFI:")) {
            // Parse: WIFI:status:saveData:timestamp
            int parts[4];
            int partIndex = 0;
            int lastIndex = 5; // Start after "WIFI:"
            
            for (int i = 0; i < 3; i++) { // For status, saveData, timestamp
                int nextIndex = wifiStatus.indexOf(':', lastIndex);
                if (nextIndex == -1) nextIndex = wifiStatus.length();
                parts[partIndex++] = wifiStatus.substring(lastIndex, nextIndex).toInt();
                lastIndex = nextIndex + 1;
            }
            
            wifiConnected = (parts[0] == 1);
            saveData = parts[1];
            // parts[2] contains timestamp (can be used for message freshness)
        }
    }
}



// ================== IMPROVED CORE FUNCTIONS ================== //

void handleBuzzer() {
  if (dangerProximity > 0) {
    if (dangerProximity == 1.0) {
      // Continuous "beeeeeeeeeeeeeep" when danger is max
      if (millis() - buzzerPreviousMillis >= 1000) {
        buzzerPreviousMillis = millis();
        buzzerState = !buzzerState;
        digitalWrite(buzz, buzzerState);
      }
    } else {
      // Regular "beep beep beep" at standard interval
      if (millis() - buzzerPreviousMillis >= currentBuzzerInterval) {
        buzzerPreviousMillis = millis();
        buzzerState = !buzzerState;
        digitalWrite(buzz, buzzerState);
      }
    }

    // Update warning display
    clearLine(0);  // Clear the first line
    lcd.setCursor(0, 0);
    lcd.print(dangerSensor);
  } else {
    digitalWrite(buzz, LOW);
    clearLine(0);
  }
}

void checkDangerLevels() {
  dangerProximity = 0.0;
  dangerSensor = "";
  
  // pH Monitoring
  if (ph_act <= PH_DANGER_LOW || ph_act >= PH_DANGER_HIGH) {
    dangerSensor += "pH DANGER!";
    dangerProximity = max(dangerProximity, 1.0);
  } 
  else if (ph_act <= PH_WARNING_LOW || ph_act >= PH_WARNING_HIGH) {
    float phProximity = (ph_act <= PH_WARNING_LOW) ? 
      mapFloat(ph_act, PH_DANGER_LOW, PH_WARNING_LOW, 1.0, 0.0) :
      mapFloat(ph_act, PH_WARNING_HIGH, PH_DANGER_HIGH, 0.0, 1.0);
      
    if (phProximity > 0.0) {
      dangerSensor += "pH Warn";
      dangerProximity = max(dangerProximity, phProximity);
    }
  }

  // Turbidity Monitoring
  if (turbidity >= TURB_DANGER) {
    dangerSensor += "TURB DANGER!";
    dangerProximity = max(dangerProximity, 1.0);
  } 
  else if (turbidity >= TURB_WARNING) {
    float turbProximity = mapFloat(turbidity, TURB_WARNING, TURB_DANGER, 0.0, 1.0);
    if (turbProximity > 0.0) {
      dangerSensor += "TURB High";
      dangerProximity = max(dangerProximity, turbProximity);
    }
  }

  // Temperature Monitoring
  if (water_temp <= TEMP_DANGER_LOW || water_temp >= TEMP_DANGER_HIGH) {
    dangerSensor += "TEMP DANGER!";
    dangerProximity = max(dangerProximity, 1.0);
  } 
  else if (water_temp <= TEMP_WARNING_LOW || water_temp >= TEMP_WARNING_HIGH) {
    float tempProximity = (water_temp <= TEMP_WARNING_LOW) ? 
      mapFloat(water_temp, TEMP_DANGER_LOW, TEMP_WARNING_LOW, 1.0, 0.0) :
      mapFloat(water_temp, TEMP_WARNING_HIGH, TEMP_DANGER_HIGH, 0.0, 1.0);
      
    if (tempProximity > 0.0) {
        dangerSensor += "TEMP Warn";
        dangerProximity = max(dangerProximity, tempProximity);
    }
  }

  // Update buzzer speed (1000ms to 200ms)
  currentBuzzerInterval = 1000 - (800 * dangerProximity);
  if (currentBuzzerInterval < 200) currentBuzzerInterval = 200;  // Min buzzer interval

  // Flashing logic (alternate visibility)
  if (dangerProximity > 0) {
    flashMillis = millis();
    isFlashing = !isFlashing;
  }
}

void display_data() {
  // WiFi status icon
  lcd.setCursor(15, 3);
  if (wifiConnected) {
    lcd.write(byte(0));  // WiFi icon
  } else {
    lcd.write(byte(1));  // No WiFi icon
  }

    // Always show saveData (moved outside the else block)
  lcd.setCursor(17, 3);
  lcd.write(byte(2));  // Data save icon
  lcd.setCursor(18, 3);
  if (saveData > 0) {
    lcd.print(saveData);
  } else {
    lcd.print("0");
  }

  

  // Flashing sensor indicators
  String phStatus = (ph_act >= PH_SAFE_MIN && ph_act <= PH_SAFE_MAX) ? 
                   ((ph_act >= PH_WARNING_LOW && ph_act <= PH_WARNING_HIGH) ? "[OK]" : "[!]") : "[X]";
  
  String turbStatus = (turbidity <= TURB_SAFE_MAX) ? 
                    ((turbidity <= TURB_WARNING) ? "[OK]" : "[!]") : "[X]";
  
  String tempStatus = (water_temp >= TEMP_SAFE_MIN && water_temp <= TEMP_SAFE_MAX) ? 
                    ((water_temp >= TEMP_WARNING_LOW && water_temp <= TEMP_WARNING_HIGH) ? "[OK]" : "[!]") : "[X]";

  // LCD Output (20x4 format)
  lcd.setCursor(0, 1);
  lcd.print("pH:" + String(ph_act,2) + phStatus);
  
  lcd.setCursor(0, 2);
  lcd.print("Turb:" + String(turbidity) + turbStatus);
  
  lcd.setCursor(0, 3);
  lcd.print("Temp:" + String(water_temp,1) + tempStatus);

  // Flash danger sensor if needed
  if (dangerProximity > 0 && isFlashing) {
    clearLine(0);  // Clear the first line before flashing
    lcd.setCursor(0, 0);
    lcd.print(dangerSensor);  // Flashing warning
  }
}

void clearLine(int line) {
  lcd.setCursor(0, line);
  for (int i = 0; i < 19; i++) lcd.print(" ");
  lcd.setCursor(0, line);
}

// ================== SENSOR FUNCTIONS ================== //

void ph_readings() {
  // Improved pH reading with moving average
  static float ph_readings[5];
  static byte index = 0;

  int avgval = 0;
  
  for (int i = 0; i < 10; i++) {
    buffer_arr[i] = analogRead(ph_pin);
    delay(30);
  }
  
  // Sort and average middle 6 values
  for (int i = 0; i < 9; i++) {
    for (int j = i+1; j < 10; j++) {
      if (buffer_arr[i] > buffer_arr[j]) {
        temp = buffer_arr[i];
        buffer_arr[i] = buffer_arr[j];
        buffer_arr[j] = temp;
      }
    }
  }
  
  avgval = 0;
  for (int i = 2; i < 8; i++) avgval += buffer_arr[i];
  
  float volt = (float)avgval * 5.0 / 1023.0 / 6;
  ph_readings[index] = -5.70 * volt + calibration_value;
  
  index = (index + 1) % 5;
  ph_act = 0;
  for (int i = 0; i < 5; i++) ph_act += ph_readings[i];
  ph_act /= 5;
}

void turb_reading() {
  // Smoothed turbidity reading
  sensorValue = 0;
  for (int i = 0; i < 5; i++) {
    sensorValue += analogRead(turb_pin);
    delay(10);
  }
  sensorValue /= 5;
  turbidity = map(sensorValue, 0, 977, 300, 0);
}

void temp_read() {
  temp_sensor.requestTemperatures();
  water_temp = temp_sensor.getTempCByIndex(0);
}

// ================== UTILITY FUNCTIONS ================== //

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
