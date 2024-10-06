#include <TFT_eSPI.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Orbitron_Medium_20.h"

// Definizione dei colori in formato RGB565
#define BLACK   0x0000
#define WHITE   0xFFFF
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F
#define YELLOW  0xFFE0

// Inizializzazione display
TFT_eSPI tft = TFT_eSPI();

// Definizione di nomi brevi per i font
#define M20 &Orbitron_Medium_20

// Costanti per il display e l'interfaccia
const int DISPLAY_WIDTH = 320;
const int DISPLAY_HEIGHT = 240;

// Credenziali WiFi
const char* ssid = "NETGEAR";                // input SSID WIFI
const char* password = "Password";           // WIFI PASSWORD
const char* shellyApiUrl = "http://192.168.0.2/status";   // SHELLY EM IP

// Costanti per l'impianto fotovoltaico
#define UPDATE_INTERVAL 5000          // Intervallo di aggiornamento in ms
#define MAX_POWER 6000                // INPUT MAX POWER: ie IN MY HOME 6 KW

// Variabili globali e modalità TEST
uint32_t lastUpdateTime = 0;
bool testMode = false;    // Attiva o disattiva la modalita' TEST false

float lastDisplayValue = -1;
char lastUnit[10] = "";
float lastPercentage = -1;

// Struttura per i dati di potenza: pannelli, rete, uso (consumo)
struct PowerData {
    float p0, p1, p2;
};

// Dichiarazioni anticipate delle funzioni
void drawText(const char* text, int x, int y, const GFXfont* font, int size, uint16_t color, bool center = false);
void clearAndDrawText(const char* text, int x, int y, const GFXfont* font, int size, uint16_t color, bool center = false, uint16_t bgColor = BLACK, bool isPercentage = false);
void drawArc(int x, int y, int start_angle, int end_angle, int r, int thickness, uint32_t color);
uint32_t getSpecificColor(float ratio);
void drawPowerGauge(float value, const GFXfont* valueFont, int valueSize, const GFXfont* unitFont, int unitSize, const GFXfont* percentFont, int percentSize);
void connectToWiFi();
String getShellyData();
PowerData getRealPowerData();
void formatPowerValue(char* buffer, size_t bufferSize, float value);
void displayPowerValues(float p1, float p2, int p1Size, int p2Size);
PowerData getTestPowerData();
void drawBaseInterface();

void setup() {
  tft.init();
  tft.setRotation(1);       // Rotazione del display
  tft.fillScreen(BLACK);
  
  connectToWiFi();
  drawBaseInterface();      // Disegno dell'interfaccia di base
}

void loop() {
// Aggiornamento ogni UPDATE_INTERVAL millisecondi
  if (millis() - lastUpdateTime >= UPDATE_INTERVAL) {
    lastUpdateTime = millis();
    
    PowerData power = testMode ? getTestPowerData() : getRealPowerData();
    
    // Disegno del gauge e aggiornamento dei valori
    drawPowerGauge(power.p0, M20, 2, M20, 1, M20, 1); // Dati del gauge, font e dimensione per dato numero e font per dato testuale ( Kw o watt)
    displayPowerValues(power.p1, power.p2, 1, 1); // Specifica le dimensioni del testo: 2 per p1, 3 per p2
  }
}

// Disegna l'interfaccia di base del display
void drawBaseInterface() {
  drawText("Pannelli", 7, 232, M20, 2, TFT_ORANGE);
}

// Disegna un arco sul display
void drawArc(int x, int y, int start_angle, int end_angle, int r, int thickness, uint32_t color) {
  float start_angle_rad = start_angle * DEG_TO_RAD;
  float end_angle_rad = end_angle * DEG_TO_RAD;
  float step = 0.08;  // modificato da 0.01 a 0.1 a 0.08

  for (float i = start_angle_rad; i <= end_angle_rad; i += step) {
    float sx = cos(i), sy = sin(i);
    uint16_t x0 = sx * r + x, y0 = sy * r + y;
    uint16_t x1 = sx * (r - thickness) + x, y1 = sy * (r - thickness) + y;
    tft.drawLine(x0, y0, x1, y1, color);
  }
}

// Ottiene un colore specifico basato su un rapporto (0-1)
uint32_t getSpecificColor(float ratio) {
  const uint32_t colors[] = {
    tft.color565(255, 0, 0),    // Rosso
    tft.color565(255, 165, 0),  // Arancione
    tft.color565(255, 255, 0),  // Giallo
    tft.color565(0, 255, 0),    // Verde
    tft.color565(0, 0, 255)     // Blu
  };
  const int numColors = 5;
  
  float scaledRatio = ratio * (numColors - 1);
  int index = floor(scaledRatio);
  float fraction = scaledRatio - index;
  
  if (index >= numColors - 1) return colors[numColors - 1];
  
  // Interpolazione lineare tra due colori
  uint8_t r1 = (colors[index] >> 11) & 0x1F;
  uint8_t g1 = (colors[index] >> 5) & 0x3F;
  uint8_t b1 = colors[index] & 0x1F;
  
  uint8_t r2 = (colors[index + 1] >> 11) & 0x1F;
  uint8_t g2 = (colors[index + 1] >> 5) & 0x3F;
  uint8_t b2 = colors[index + 1] & 0x1F;
  
  uint8_t r = r1 + (r2 - r1) * fraction;
  uint8_t g = g1 + (g2 - g1) * fraction;
  uint8_t b = b1 + (b2 - b1) * fraction;
  
  return tft.color565(r << 3, g << 2, b << 3);
}

void drawPowerGauge(float value, const GFXfont* valueFont, int valueSize, const GFXfont* unitFont, int unitSize, const GFXfont* percentFont, int percentSize) {
  int x = 105, y = 100, radius = 95;
  int angleRange = 150;
  int startAngle = 270 - angleRange;
  int endAngle = 270 + angleRange;
  int mappedValue = map(value, 0, MAX_POWER, startAngle, endAngle);
  
  // Calcolo della percentuale
  float percentage = (value / MAX_POWER) * 100;
  char percentBuf[10];
  snprintf(percentBuf, sizeof(percentBuf), "%.1f%%", percentage);

  // Pulisci l'area del gauge
  tft.fillRect(x - radius, y - radius, radius * 2, radius * 2, BLACK);
 
  // Disegno del semicerchio esterno con bordo blu e riempimento trasparente
  drawArc(x, y, startAngle, endAngle, radius, 1, BLUE);
  drawArc(x, y, startAngle, endAngle, radius - 26, 1, BLUE);
 
  // Disegno del semicerchio interno con colore di riempimento basato sul valore
  for (int angle = startAngle; angle <= mappedValue; angle++) {
    float ratio = float(angle - startAngle) / (endAngle - startAngle);
    uint32_t color = getSpecificColor(ratio);
    drawArc(x, y, angle, angle + 1, radius - 3, 20, color);
  }
 
  // Prepara il valore numerico e l'unità di misura
  char valueBuf[20], unitBuf[5];
  if (value >= 1000) {
    snprintf(valueBuf, sizeof(valueBuf), "%.2f", value / 1000.0);
    strcpy(unitBuf, "kW");
  } else {
    snprintf(valueBuf, sizeof(valueBuf), "%d", (int)round(value));
    strcpy(unitBuf, "Watt");
  }
  
  // Visualizza i dati
  clearAndDrawText(valueBuf, x, y - 15, valueFont, valueSize, WHITE, true);
  clearAndDrawText(unitBuf, x, y + 75, unitFont, unitSize, WHITE, true);

  // Posizione della percentuale
  int percentX = x + 160;
  int percentY = y + 20 + radius;

  // Usa clearAndDrawText con un'area di pulizia più larga per la percentuale
  clearAndDrawText(percentBuf, percentX, percentY, percentFont, percentSize, WHITE, true, BLACK, true);
   
  // Aggiorna i valori memorizzati cache
  lastDisplayValue = value;
  strcpy(lastUnit, unitBuf);
  lastPercentage = percentage;

  // Ridisegna il bordo completo e le linee interne
  tft.drawRect(0, 0, DISPLAY_WIDTH, DISPLAY_HEIGHT, BLUE);
  tft.fillRect(213, 0, 2, 240, BLUE);
  tft.fillRect(213, 98, 320, 2, BLUE);
  tft.fillRect(0, 197, 320, 2, BLUE);
}

// Connette il dispositivo al WiFi
void connectToWiFi() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");
}

// Ottiene i dati dal dispositivo Shelly
String getShellyData() {
  HTTPClient http;
  http.begin(shellyApiUrl);
  int httpCode = http.GET();
  
  if (httpCode > 0) {
    String payload = http.getString();
    http.end();
    return payload;
  } else {
    http.end();
    return "";
  }
}

// Elabora i dati JSON ottenuti da Shelly
PowerData getRealPowerData() {
  String jsonData = getShellyData();
  PowerData result = {-1, -1, -1};  // Valori di errore predefiniti

  if (jsonData.length() == 0) return result;

  StaticJsonDocument<1024> doc;
  DeserializationError error = deserializeJson(doc, jsonData);
  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return result;
  }

  result.p0 = doc["emeters"][0]["power"];
  result.p1 = doc["emeters"][1]["power"];
  result.p2 = result.p0 + result.p1;
  return result;
}

void drawText(const char* text, int x, int y, const GFXfont* font, int size, uint16_t color, bool center) {
  tft.setTextColor(color);
  
  if (font) {
    tft.setFreeFont(font);
  } else {
    tft.setTextFont(1); // Font predefinito
  }
  
  tft.setTextSize(size);
 
  int16_t textWidth = tft.textWidth(text);
  int16_t textHeight = tft.fontHeight();
 
  int xPos = x;
  int yPos = y;
 
  if (center) {
    xPos -= textWidth / 2;
    yPos -= textHeight / 2;
  }

  tft.setCursor(xPos, yPos);
  tft.print(text);

  if (font) {
    tft.setTextFont(1);
  }
}

// Funzione di supporto per formattare il valore di potenza
void formatPowerValue(char* valueBuf, size_t valueBufferSize, char* unitBuf, size_t unitBufferSize, float value) {
  if (value >= 1000 || value <= -1000) {
    snprintf(valueBuf, valueBufferSize, "%.2f", value / 1000.0);
    strncpy(unitBuf, "kW", unitBufferSize);
  } else {
    snprintf(valueBuf, valueBufferSize, "%d", (int)round(value));
    strncpy(unitBuf, "Watt", unitBufferSize);
  }
}

// Funzione aggiornata per visualizzare i valori di potenza
void displayPowerValues(float p1, float p2, int p1Size, int p2Size) {
    char valueBuf[20];
    char unitBuf[5];
    
    // Coordinate per "Rete" e "Uso"
    int reteX = 235, reteY = 35, reteValueOffsetY = 30, unitOffsetY = 25;
    int usoX = 235, usoY = 132, usoValueOffsetY = 30;
    
    clearAndDrawText("Rete", reteX, reteY, M20, 1, TFT_ORANGE, false, BLACK, true);
    
    formatPowerValue(valueBuf, sizeof(valueBuf), unitBuf, sizeof(unitBuf), p1);
    clearAndDrawText(valueBuf, reteX, reteY + reteValueOffsetY, M20, p1Size, WHITE, false, BLACK, true);
    clearAndDrawText(unitBuf, reteX, reteY + reteValueOffsetY + unitOffsetY, M20, 1, WHITE, false, BLACK, true);

    clearAndDrawText("Uso", usoX, usoY, M20, 1, TFT_ORANGE, false, BLACK, true);

    formatPowerValue(valueBuf, sizeof(valueBuf), unitBuf, sizeof(unitBuf), p2);
    clearAndDrawText(valueBuf, usoX, usoY + usoValueOffsetY, M20, p2Size, WHITE, false, BLACK, true);
    clearAndDrawText(unitBuf, usoX, usoY + usoValueOffsetY + unitOffsetY, M20, 1, WHITE, false, BLACK, true);
}

// Genera valori casuali se in modalita' test
PowerData getTestPowerData() {
  PowerData power;
  power.p0 = random(0, MAX_POWER);
  power.p1 = random(0, MAX_POWER);
  power.p2 = power.p0 + power.p1;
  return power;
}

// Funzione aggiornata per pulire l'area e disegnare il testo
void clearAndDrawText(const char* text, int x, int y, const GFXfont* font, int size, uint16_t color, bool center, uint16_t bgColor, bool isPercentage) {
  tft.setTextColor(color);
  
  if (font) {
    tft.setFreeFont(font);
  } else {
    tft.setTextFont(1);
  }
  
  tft.setTextSize(size);
 
  int16_t textWidth = tft.textWidth(text);
  int16_t textHeight = tft.fontHeight();
 
  int marginX = isPercentage ? 10 : 6;  // Margine orizzontale molto più grande per la percentuale
  int marginY = 3;  // Margine verticale standard
  
  int xPos = x;
  int yPos = y;
 
  if (center) {
    xPos -= textWidth / 2;
    yPos += textHeight / 2;
  }

  // Aumenta significativamente la larghezza dell'area di pulizia per la percentuale
  int cleanWidth = isPercentage ? textWidth + 4 * marginX : textWidth + 2 * marginX;
  int cleanHeight = textHeight + 2 * marginY;

  // Pulisce un'area più larga del testo, specialmente per la percentuale
  tft.fillRect(xPos - (isPercentage ? 2 * marginX : marginX), 
               yPos - textHeight - marginY, 
               cleanWidth, 
               cleanHeight, 
               bgColor);
 
  // Disegna il nuovo testo
  tft.setCursor(xPos, yPos);
  tft.print(text);

  if (font) {
    tft.setTextFont(1);
  }
}