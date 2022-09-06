#include <UMS3.h>
UMS3 ums3;

#include <esp_now.h>
#include <WiFi.h>

//#define USE_ASYNC_UPDATES

#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>

//SCREEN PINS
#define SCREEN_CS   34
#define SCREEN_RST  9 
#define SCREEN_DC   8
//Create TFT object with correct pins
Adafruit_ST7789 tft = Adafruit_ST7789(SCREEN_CS, SCREEN_DC, SCREEN_RST);

uint16_t boxColor = 0x76D8;
uint16_t axisColor = 0x2207;

//JOYSTICK PINS
#define JOY_V  A0
#define JOY_H  A2 
#define JOY_SW A1
int joystick_x = 128;
int joystick_y = 128;
bool joystick_sw = 0;
int lastPosX = 0;
int lastPosY = 0;

//BATTERY PINS
float remoteBatteryVoltage = 0.0;
float lastRemoteBatteryVoltage = 0.0;
float remoteBatteryVoltageOffset = 0.0;

//BOARD BATTERY
byte boardBatteryCells = 10;
float boardBatteryVoltage = 0.0;
float lastBoardBatteryVoltage = 0.0;
float boardBatteryVoltageOffset = 0.0;

//BOARD ODOMETRY
int8_t mph = 0;
int8_t lastMph = 0;
int totalMiles = 69;
int tripMiles = 240;
const float rpmScaling = ((((90.0/25.4)/12.0)/5280.0)*3.14)*60.0;
const float motorPolePairs = 10.0;

//RADIO SETUP
//A: F4:12:FA:4F:A5:B0
uint8_t broadcastAddress[] = {0xF4, 0x12, 0xFA, 0x4F, 0xA5, 0xB0};

//Struct for holding board telemetry
typedef struct TelemetryStruct {
  float voltage;
  long rpm;
  long tach_abs;
} TelemetryStruct;

TelemetryStruct telemetry;
esp_now_peer_info_t peerInfo;

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
//  Serial.print("\r\nLast Packet Send Status:\t");
//  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
//  if (status ==0){
//    success = "Delivery Success :)";
//  }
//  else{
//    success = "Delivery Fail :(";
//  }
}

// Callback when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&telemetry, incomingData, sizeof(telemetry));
  boardBatteryVoltage = telemetry.voltage;
  mph = telemetry.rpm/motorPolePairs*rpmScaling;
  tripMiles = telemetry.tach_abs;
}

void boardBatteryDisplay(){
  if (abs(boardBatteryVoltage-lastBoardBatteryVoltage)>0.1){
    tft.fillRoundRect(1, 0, tft.width()-2, 4, 2, ST77XX_BLACK);
    tft.drawRoundRect(1, 0, tft.width()-2, 4, 2, ST77XX_GREEN);
    
    int fillamount = boardBatteryVoltage/(4.2*boardBatteryCells)*(tft.width()-2);
    uint16_t color = 0;
    if (boardBatteryVoltage > (3.8*boardBatteryCells) and boardBatteryVoltage <= (4.2*boardBatteryCells)){
      color = ST77XX_GREEN;
    }
    else if (boardBatteryVoltage > (3.4*boardBatteryCells) and boardBatteryVoltage <= (3.8*boardBatteryCells)){
      color = ST77XX_YELLOW;
    }
    else if (boardBatteryVoltage <= (3.4*boardBatteryCells)){
      color = ST77XX_RED;
    }
    tft.fillRoundRect(1, 0, fillamount, 4, 2, color); 
  
    lastBoardBatteryVoltage = boardBatteryVoltage;
  }
}

void remoteBatteryDisplay(){
  remoteBatteryVoltage = ums3.getBatteryVoltage()+remoteBatteryVoltageOffset;
  
  if (abs(remoteBatteryVoltage-lastRemoteBatteryVoltage)>0.1){
    tft.fillRoundRect(tft.width()/3, 6, tft.width()/3, 4, 2, ST77XX_BLACK);
    tft.drawRoundRect(tft.width()/3, 6, tft.width()/3, 4, 2, ST77XX_GREEN);
    
    int fillamount = remoteBatteryVoltage/4.2*(tft.width()/3);
    uint16_t color = 0;
    if (remoteBatteryVoltage > 3.8 and remoteBatteryVoltage <= 4.2){
      color = ST77XX_GREEN;
    }
    else if (remoteBatteryVoltage > 3.4 and remoteBatteryVoltage <= 3.8){
      color = ST77XX_YELLOW;
    }
    else if (remoteBatteryVoltage <= 3.4){
      color = ST77XX_RED;
    }
    tft.fillRoundRect(tft.width()/3, 6, fillamount, 4, 2, color); 
  
    lastRemoteBatteryVoltage = remoteBatteryVoltage;
  }
}

void joystickDisplay(){
  tft.drawRoundRect(tft.width()/2+2, tft.height()/3-25, tft.width()/2-2, tft.height()/1.8, 5, boxColor);
  tft.drawFastVLine(3*tft.width()/4, tft.height()/3-25, tft.height()/1.8, axisColor);
  tft.drawFastHLine(tft.width()/2+2, (tft.height()/3-25) + (tft.height()/1.8)/2, tft.width()/2-2, axisColor);

  int centerY = (tft.height()/3-25) + (tft.height()/1.8)/2;
  int centerX = 3*tft.width()/4;
  int pos_x = map(joystick_x,0,255,centerX-(tft.width()/2)/2+8,centerX+(tft.width()/2-2)/2-8);
  int pos_y = map(joystick_y,0,255,centerY+(tft.height()/1.8)/2-8,centerY-(tft.height()/1.8)/2+8);

  if ((pos_x != lastPosX) or (pos_y!= lastPosY)){
    tft.fillCircle(lastPosX,lastPosY,5,ST77XX_BLACK);
    lastPosX = pos_x;
    lastPosY = pos_y;
  }
  tft.fillCircle(pos_x,pos_y,5,ST77XX_RED);
}

void readInputs(){
  joystick_x = map(analogRead(JOY_H),660,3220,0,255);
  if(joystick_x >= 255){
    joystick_x = 255;
  }
  if(joystick_x <= 0){
    joystick_x = 0;
  } 
  
  joystick_y = map(analogRead(JOY_V),630,3220 ,255, 0);
  if(joystick_y >= 255){
    joystick_y = 255;
  }
  if(joystick_y <= 0){
    joystick_y = 0;
  } 

  joystick_sw = digitalRead(JOY_SW);
}

void odometryDisplay(){
  byte speedLocation;
  tft.setTextSize(8);
  
  if (lastMph != mph){
    if (lastMph > 9 or lastMph < 0){
      speedLocation = tft.width()/5-35;
    }
    else{
      speedLocation = tft.width()/5-10;
    }
    tft.setCursor(speedLocation, tft.height()/3-25);
    tft.setTextColor(ST77XX_BLACK);
    tft.print(lastMph);
    lastMph = mph;
  }
  
  if (mph > 9 or mph < 0){
    speedLocation = tft.width()/5-35;
  }
  else{
    speedLocation = tft.width()/5-10;
  }
  tft.setCursor(speedLocation, tft.height()/3-25);
  tft.setTextColor(ST77XX_WHITE);
  tft.print(mph);

  tft.setTextSize(2);
  tft.setCursor(tft.width()/5-7, 4*tft.height()/5-27);
  tft.print("MPH");
  
  tft.drawRoundRect(2, 3*tft.height()/4, tft.width()/2-2, tft.height()/4, 5, boxColor);
  tft.drawRoundRect(tft.width()/2+2, 3*tft.height()/4, tft.width()/2-2, tft.height()/4, 5, boxColor);

  tft.setTextSize(0);
  tft.setCursor(tft.width()/4-15, 3*tft.height()/4+3);
  tft.print("TOTAL");
  tft.setTextSize(1);
  tft.setCursor(tft.width()/4-15, 3*tft.height()/4+20);
  tft.print(totalMiles);

  tft.setTextSize(0);
  tft.setCursor(3*tft.width()/4-11, 3*tft.height()/4+3);
  tft.print("TRIP");
  tft.setTextSize(1);
  tft.setCursor(3*tft.width()/4-11, 3*tft.height()/4+20);
  tft.print(tripMiles);
}


void setup() {
  //Initialize all board peripherals
  ums3.begin();

  //Confugure Peer Communication
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    return;
  }

  //Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Trasnmitted packet
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    return;
  }
  
  // Register for a callback function that will be called when data is received
  esp_now_register_recv_cb(OnDataRecv);

  Serial.begin(115200);
  
  //Configure Joystick Pins
  pinMode(JOY_V,INPUT);
  pinMode(JOY_H,INPUT);
  pinMode(JOY_SW,INPUT);

  //Initialize screen and set to black
  tft.init(135, 240);
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  joystickDisplay();
  odometryDisplay();
}

long lastbatterytime = millis();
long lastanimatetime = millis();
long lastinputtime = millis();
long lastodomtime = millis();
void loop() {
  if (millis()-lastinputtime > 10){
    readInputs();
    Serial.println(joystick_y);
    lastinputtime = millis();
  }
  if (millis()-lastanimatetime > 25){
    joystickDisplay();
    lastanimatetime = millis();
  }
  if (millis()-lastbatterytime > 1000){
    boardBatteryDisplay();
    remoteBatteryDisplay();
    lastbatterytime = millis();
  }
  if (millis()-lastodomtime > 250){
    odometryDisplay();
    lastodomtime = millis();
  }

  byte outputValue =  joystick_y;
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &outputValue, sizeof(outputValue));
  
//  radio.stopListening();
//  byte outputValue =  joystick_y;
//  radio.write(&outputValue, sizeof(outputValue));
//  
//  uint8_t pipe;
//  if (radio.available(&pipe)) {
//    TelemetryStruct received;
//    radio.read(&received, sizeof(received));
//    boardBatteryVoltage = received.voltage;
//    mph = received.rpm/motorPolePairs*rpmScaling;
//    tripMiles = received.tach_abs;
////    Serial2.print(F(" Recieved ")); 
////    Serial2.print(radio.getDynamicPayloadSize());
////    Serial2.print(F(" bytes on pipe "));
////    Serial2.print(pipe);
////    Serial2.print(F(": "));
////    Serial2.println(received.rpm );
//  }   
}
