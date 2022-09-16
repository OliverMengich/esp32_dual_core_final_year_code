#include <WiFi.h>
#include <Firebase_ESP_Client.h>

#include <TFT_eSPI.h> // Hardware-specific library
#include <SPI.h>
#define BLACK_SPOT

TFT_eSPI tft = TFT_eSPI();       // Invoke custom library
#define CF_OL24 &Orbitron_Light_24
#define CF_RT24 &Orbitron_Light_24
#define GFXFF 1
#define FF18 &FreeSans12pt7b
#include "Free_Fonts.h" // Include the header file attached to this sketch
unsigned long drawTime = 0;
#define FRAME_X 100
#define FRAME_Y 64
#define FRAME_W 120
#define FRAME_H 50
bool SwitchOn = false;
bool returnHome=false;
// Red zone size
#define REDBUTTON_X FRAME_X
#define REDBUTTON_Y FRAME_Y
#define REDBUTTON_W (FRAME_W/2)
#define REDBUTTON_H FRAME_H

#define GREENBUTTON_X (REDBUTTON_X + REDBUTTON_W)
#define GREENBUTTON_Y FRAME_Y
#define GREENBUTTON_W (FRAME_W/2)
#define GREENBUTTON_H FRAME_H
//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "One Love"
#define WIFI_PASSWORD "1231oneheart"

// Insert Firebase project API Key
#define API_KEY "AIzaSyB_kFgekXTyQQehULvCQT7zcKqZoGQf5Tc"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://finalyearproject-af718-default-rtdb.firebaseio.com/"  
//Define Firebase Data object
FirebaseData fdbo;

FirebaseAuth auth;
FirebaseConfig config;
FirebaseJson json;

// Parent Node (to be updated in every loop)
// String parentPath;

float defaultcurrVal;
float defaultvoltVal;
float defaultPotVal = 0;
//Create a task with the name Task1 and Task2
TaskHandle_t Task1;
TaskHandle_t Task2;
int value = 0;
int mapped = 0;
// create a global mutex
static SemaphoreHandle_t mutex;
//these are the pins to be used to generates PWM signals that power

long duration;
float distanceCm;
#define SOUND_SPEED 0.034
//pins for USS1, USS2 and USS3
const int trigPin1 = 34;
const int echoPin1 = 35;

const int trigPin2 = 32;
const int echoPin2 = 33;

const int trigPin3 = 27;
const int echoPin3 = 14;
//pins for Voltage sensor and current sensor
const int currPin = 36;
const int voltPin = 39;
//pins for FAN and temperature sensor
const int fanFet = 22;
const int temp = 12;

const int potpin = 13;
void setup(){
  initPins(); 
  Serial.begin(115200); 
  initWifi();
  initFireBase();

  tft.init();
  tft.setRotation(1);
  Serial.println("LCD INITIALIZED");
  //create Mutex
  mutex = xSemaphoreCreateMutex(); // the ultrasonic sensors and potentiometers need mutex
  //initialize the cores
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */
   delay(500);                 
  xTaskCreatePinnedToCore(
                    Task2code,   /* Task function. */
                    "Task2",     /* name of task. */
                    10000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task2,      /* Task handle to keep track of created task */
                    1);          /* pin task to core 1 */
   delay(500);
  homePage();
  Serial.println("I'm done with All initialization");
}
void drawDatumMarker(int x, int y){
  tft.drawLine(x,y, x + 10, y, TFT_GREEN);
  tft.drawLine(x, y, x+5, y - 5, TFT_GREEN);
  tft.drawLine(x, y, x+5, y + 5, TFT_GREEN);
}
void header(const char *string, uint16_t color){
  tft.fillScreen(color);
  tft.setTextSize(1);
  tft.setTextColor(TFT_MAGENTA, TFT_BLUE);
  tft.fillRect(0, 0, 320, 30, TFT_BLUE);
  tft.setTextDatum(TC_DATUM);
  tft.drawString(string, 160, 2, 4); // Font 4 for fast drawing with background
}
void homePage(){
  header("SMART ENERGY METER", TFT_BLACK); 
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setFreeFont(FF18);                 // Select the font
  tft.setFreeFont(CF_OL24);
  //Configuration Page
  tft.drawLine(10,50, 300, 50, TFT_GREEN);
  tft.drawLine(300,50, 300, 90, TFT_GREEN);
  tft.drawLine(300,90, 10, 90, TFT_GREEN);
  tft.drawLine(10,90, 10, 50, TFT_GREEN);
  tft.setCursor(15, 75);
  tft.print("Configurations Page");
  //Online Page
  tft.drawLine(10,100, 230, 100, TFT_GREEN);
  tft.drawLine(230,100, 230, 135, TFT_GREEN);
  tft.drawLine(230,135, 10, 135, TFT_GREEN);
  tft.drawLine(10,135, 10, 50, TFT_GREEN);
  tft.setCursor(15, 125);
  tft.print("Readings Page");
  // Infor Page
  tft.drawLine(10,150, 230, 150, TFT_GREEN);
  tft.drawLine(230,100, 230, 185, TFT_GREEN);
  tft.drawLine(230,185, 10, 185, TFT_GREEN);
  tft.drawLine(10,185, 10, 50, TFT_GREEN);
  tft.setCursor(15, 175);
  tft.print("Power Page");
}
void initPins(){
    pinMode(13,INPUT);
    pinMode(25, OUTPUT);
    pinMode(26, OUTPUT);
    pinMode(36,INPUT);
    pinMode(39,INPUT);
    pinMode(trigPin1, OUTPUT);
    pinMode(echoPin1, INPUT);
    pinMode(trigPin2, OUTPUT);
    pinMode(echoPin2, INPUT);
    pinMode(trigPin3, OUTPUT);
    pinMode(echoPin3, INPUT);
}
void initWifi(){
    // set a timer for 10 seconds to see if the access point is active. If not skip trying to connect
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED){
      Serial.print(".");
      vTaskDelay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();
}
void initFireBase(){
    config.api_key = API_KEY;

    /* Assign the RTDB URL (required) */
    config.database_url = DATABASE_URL;

    /* Sign up */
    if (Firebase.signUp(&config, &auth, "", "")){
      Serial.println("ok ;-) Connected to the Database");
    }else{
      Serial.print("Error Encountered");
      Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    return;
}
float measureUSS(int trigPin,int echoPin){
    digitalWrite(trigPin, LOW);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    // Sets the trigPin on HIGH state for 10 micro seconds
    digitalWrite(trigPin, HIGH);
    vTaskDelay(1 / portTICK_PERIOD_MS);
    digitalWrite(trigPin, LOW);
    // Reads the echoPin, returns the sound wave travel time in microseconds
    duration = pulseIn(echoPin, HIGH);
    // Calculate the distance
    distanceCm = duration * SOUND_SPEED/2;
    return distanceCm;
}
float measurecurrSensor(){
  float max_val = 0;
  int sensorValue1 = 0;
  int sensorValue2 = 0;
  for(int i = 0; i<100; i++){
    sensorValue1 = analogRead(currPin);
    sensorValue2 = analogRead(currPin);
    if(sensorValue2 > sensorValue1){
      max_val = sensorValue2;
    }else{
      max_val = sensorValue1;
    }
  }
  return max_val;
}
float measurevoltSensor(){
  float max_v = 0.0;
  float sensorValue1 = 0;
  float sensorValue2 = 0;
  for(int i = 0; i<100; i++){
    sensorValue1 = analogRead(voltPin);
    sensorValue2 = analogRead(voltPin);
    if(sensorValue2 > sensorValue1){
      max_v = sensorValue2;
    }else{
      max_v = sensorValue1;
    }
  }
  return max_v;
}
void Task1code( void * parameters ){
  //1. only read sensor values then there's a change in sensor reading
//  Serial.print("Task 1 running: ");
//  Serial.println(xPortGetCoreID());
  while(true){
    uint16_t x1, y1;
    if (tft.getTouch(&x1, &y1)){
      Serial.println("Nav was touched");
      Serial.println(x1);
      Serial.println(y1);
      if(x1<=35 && y1 >60 && y1<=75){
        Serial.println("Navigating to configurations page");
        configurationPage();
      }else if(x1<=35 && y1 >100 && y1 <= 125){
        Serial.println("Navigating to ReadingsPage");
        ReadingsPage();
      }
      else if(x1<=35 && y1 >130 && y1 <= 175){
        Serial.println("Navigating to Power ReadingsPage");
        powerReadingsPage();
      }
      else{
        homePage();
      }
    }
  }
}
void ReadingsPage(){
  bool hereH1=true;
  tft.setFreeFont(CF_RT24); 
  while(hereH1==true){
    float uss1Val = measureUSS(trigPin1, echoPin1);
    float uss2Val = measureUSS(trigPin2, echoPin2); 
    float uss3Val = measureUSS(trigPin3, echoPin3);
    onlineControl(uss1Val,uss2Val,uss3Val);
    uint16_t x, y;
    // See if there's any touch data for us
  if (tft.getTouch(&x, &y)){
      Serial.println("I was touched !!!!");
      Serial.println(x);
      Serial.println(y);
     if(x<=23 && y<=14){
        Serial.println("Home Button was Pressed");
        hereH1=false;
        Serial.print("HereH now is: " );
        Serial.println(hereH1);
      }
      header("Readings Page", TFT_BLACK);
      drawDatumMarker(10, 10);
      tft.setFreeFont(CF_OL24);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
    
      tft.setCursor(10,60);
      tft.print("WaterLevel 1: ");
      tft.setCursor(220,60);
      tft.print(uss1Val);
    
      tft.setCursor(10,95);
      tft.print("WaterLevel 2: ");
      tft.setCursor(220,95);
      tft.print(uss2Val);
    
      tft.setCursor(10,130);
      tft.print("WaterLevel 3: ");
      tft.setCursor(220,130);
      tft.print(uss3Val);
    
      tft.setCursor(10,160);
      tft.print("Board temp: ");
      float tempVal = analogRead(12);
      float voltage = tempVal * (5000 / 1024.0);
      tempVal = voltage/10;
      tft.setCursor(200,160);
      tft.print(tempVal);
      tft.setCursor(200,200);
      tft.print("\xC2\xB0");
      tft.setCursor(10,190);
      tft.print("Pump is: ");
      if(digitalRead(15)){
        tft.setCursor(150,190);
        tft.print("ON");
      }else{
        tft.setCursor(150,190);
        tft.print("OFF");
      }
    }
  }
  Serial.println("I'm in Readings page and I'm returning to homePage");
  return;
}
void powerReadingsPage(){
  bool hereH=true;
  tft.setFreeFont(CF_OL24);
  while(hereH==true){
    defaultcurrVal = measurecurrSensor();
    defaultvoltVal = measurevoltSensor();
    uint16_t x, y;
    // See if there's any touch data for us
    if (tft.getTouch(&x, &y)){
      Serial.println("Screen was touched !!!!");
      Serial.println(x);
      Serial.println(y);
     if(x<=23 && y<=14){
        Serial.println("Home Button was Pressed");
        hereH=false;
        Serial.print("HereH now is: " );
        Serial.println(hereH);
      }
      header("Power", TFT_BLACK);
      drawDatumMarker(10, 10);
      tft.setFreeFont(CF_OL24);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.setCursor(10,60);
      tft.print("Current: ");
      tft.setCursor(50,60);
      tft.print(defaultcurrVal);
      
      tft.setCursor(10,95);
      tft.print("Voltage: ");
      tft.setCursor(50,95);
      tft.print(defaultvoltVal);
      
      tft.setCursor(10,130);
      tft.print("POWER: ");
      tft.setCursor(50,130);
      tft.print(defaultcurrVal * defaultvoltVal);
    }
  }
  Serial.println("I'm in Power page and I'm returning to homePage");
  return;
}
void configurationPage(){
  header("Configuration Page", TFT_BLACK);
  drawDatumMarker(10, 10);
  tft.setFreeFont(CF_RT24);
  if(SwitchOn){
    greenBtn();
  }else{
    redBtn();
  }
  bool hereH=true;
  while(hereH==true){
  uint16_t x, y;
    // See if there's any touch data for us
  if (tft.getTouch(&x, &y)){
      Serial.println("Screen was touched !!!!");
      Serial.println(x);
      Serial.println(y);
       if(x<=23 && y<=14){
          Serial.println("Home Button was Pressed");
          hereH=false;
          Serial.print("HereH now is: ");
          Serial.println(hereH);
        }
      // Draw a block spot to show where touch was calculated to be
      #ifdef BLACK_SPOT
        tft.fillCircle(x, y, 2, TFT_BLACK);
      #endif
    
      if (SwitchOn){
        if ((x > REDBUTTON_X) && (x < (REDBUTTON_X + REDBUTTON_W))) {
          if ((y > REDBUTTON_Y) && (y <= (REDBUTTON_Y + REDBUTTON_H))) {
            Serial.println("Red btn hit");
            redBtn();
          }
        }
      }
      else //Record is off (SwitchOn == false)
      {
        if ((x > GREENBUTTON_X) && (x < (GREENBUTTON_X + GREENBUTTON_W))) {
          if ((y > GREENBUTTON_Y) && (y <= (GREENBUTTON_Y + GREENBUTTON_H))) {
            Serial.println("Green btn hit");
            greenBtn();
          }
        }
      }
      Serial.println(SwitchOn);
    }
  }
  Serial.println("Returning to homePage");
  return;
}
void redBtn(){
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_RED);
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("OFF", GREENBUTTON_X + (GREENBUTTON_W / 2), GREENBUTTON_Y + (GREENBUTTON_H / 2));
  SwitchOn = false;
  digitalWrite(15,LOW);
  digitalWrite(25,LOW);
  digitalWrite(26,LOW);
}

// Draw a green button
void greenBtn(){
  tft.fillRect(GREENBUTTON_X, GREENBUTTON_Y, GREENBUTTON_W, GREENBUTTON_H, TFT_GREEN);
  tft.fillRect(REDBUTTON_X, REDBUTTON_Y, REDBUTTON_W, REDBUTTON_H, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE);
  tft.setTextSize(1);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("ON", REDBUTTON_X + (REDBUTTON_W / 2) + 1, REDBUTTON_Y + (REDBUTTON_H / 2));
  SwitchOn = true;
  digitalWrite(15,HIGH);
  digitalWrite(25,HIGH);
  digitalWrite(26,HIGH);
}
void onlineControl(float uss1Val, float uss2Val, float uss3Val){
  //1. first read mutex from the cloud to check if theirs someone using the mutex
  if(Firebase.RTDB.getInt(&fdbo,"meter/mutex")){
    // if not continue
    if(fdbo.intData() == 0){
      if (uss1Val < 50 && uss1Val < 200){
        //turn on the second motor on
        if(xSemaphoreTake(mutex,0) == pdTRUE){
          fillTank(600, trigPin,echoPin);
        }
      }else if(uss2Val < 50 && uss2Val < 200){
        // take the mutex
        if(xSemaphoreTake(mutex,0) == pdTRUE){
          fillTank(400, trigPin,echoPin);
        }
      }else if (uss3Val < 50 && uss3Val < 200){
        // take the mutex
        if(xSemaphoreTake(mutex,0) == pdTRUE){
          fillTank(200, trigPin,echoPin);
       }
      }else{
        mapped=0;
      }
    }
    // if someone is using, read the value they are updating from the cloud and put to your value
    else if(fdbo.intData() == 1){
      // read motor speed from cloud
      if (Firebase.RTDB.get(&fdbo,"meter/motorSpeed")){
        int runval = fdbo.intData();
        mapped=runval;
      }
    }else{
      // display to LCD you cannot read data to cloud and user should check internet connection
      Serial.println("FAILED");
      Serial.println("REASON: " + fdbo.errorReason());
    }
  }
}
//void offlineControl(){
//  /1. Read USS values only if there's a change in value
//  float uss1Val = measureUSS(trigPin1, echoPin1);
//  float uss2Val = measureUSS(trigPin2, echoPin2);
//  float uss3Val = measureUSS(trigPin3, echoPin3);
//  // display readings on the LCD
//  int readPotValue=analogRead(potpin);
//  defaultvoltVal = measurevoltSensor();
//  if (readPotValue > defaultPotVal || readPotValue < defaultPotVal){
//    alterMotorDelay(readPotValue);
//    defaultPotVal = readPotValue;
//    //2. Read Voltage and current sensors values send values to only when there's change
//    defaultcurrVal = measurecurrSensor();
//    defaultvoltVal = ========================================================measurevoltSensor();
//    //3. Read mutex Value from cloud
//  }
//}

void fillTank(int value123, int trigPin, int echoPin){
  if(measureUSS(trigPin, echoPin) < 50 && measureUSS(trigPin,echoPin) < 200){
      if(xSemaphoreTake(mutex,0) == pdTRUE){
        if (Firebase.RTDB.set(&fdbo, "meter/mutex", 1)){
          // go ahead and motor at a constant speed
          mapped = value123;
          defaultPotVal = value123;
        }
        else{
          Serial.println("FAILED");
          Serial.println("REASON: " + fdbo.errorReason());
        }
        
    }else{
        Serial.println("Mutex is taken");
    }
  }else{
    // after finishing filling the water tank return the mutex to the cloud device
    // and give back the mutex to the microcontroller
    if (Firebase.RTDB.set(&fdbo, "meter/mutex", 0)){
      xSemaphoreGive(mutex);
      mapped=0;
    }else{
      // display to LCD you cannot set mutex value to cloud and user should check internet connection
      Serial.println("FAILED");
      Serial.println("REASON: " + fdbo.errorReason());
    }
  }
  return;
}
void Task2code( void *parameters ){
//  Serial.print("Task2 running on core ");
//  Serial.println(xPortGetCoreID());

  while(true){
    if(digitalRead(25)==1 && digitalRead(26)==1 && mapped !=0 && digitalRead(15)==1){
//      Serial.println("Pump Core started running");
      digitalWrite(25, HIGH);
      vTaskDelay(mapped/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH); 
      vTaskDelay((mapped+250)/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH);
      vTaskDelay((mapped+ 750)/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH);
      vTaskDelay((mapped+ 1500)/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH);
      vTaskDelay((mapped+750)/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH);
      vTaskDelay((mapped+250)/1000);
      digitalWrite(25, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(25, HIGH);
      vTaskDelay(mapped/1000);
      digitalWrite(25, LOW);
  
      digitalWrite(26, HIGH);
      vTaskDelay(mapped/1000);
      digitalWrite(26, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(26, HIGH);
      vTaskDelay((mapped+250)/1000);
      digitalWrite(26, LOW);
      vTaskDelay((mapped)/1000);
      digitalWrite(26, HIGH);
      vTaskDelay((mapped+750)/1000);
      digitalWrite(26, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(26, HIGH);
      vTaskDelay((mapped+1500)/1000);
      digitalWrite(26, LOW);
      vTaskDelay((mapped)/1000);
      digitalWrite(26, HIGH);
      vTaskDelay((mapped+750)/1000);
      digitalWrite(26, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(26, HIGH);
      vTaskDelay((mapped+250)/1000);
      digitalWrite(26, LOW);
      vTaskDelay(mapped/1000);
      digitalWrite(26, HIGH);
      vTaskDelay(mapped/1000);
      digitalWrite(26, LOW);
    }
  }
}
void loop(){
  
}
