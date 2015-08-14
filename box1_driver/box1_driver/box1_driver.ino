#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//box variables
const int boxID = 1;

//WIFI variables
char ssid[] = "ARDUINO";
char password[] = "ComputerScience";
int status = WL_IDLE_STATUS;

//server variables
WiFiClient client;
char server[] = "192.168.1.100";
int port = 6543;
long firstConnection;
long pingDelay = 10000;
long lastPing = 0;

//LEDs
int startLED = 26;
int finishLED = 30;
int audioLED1 = 54;
int audioLED2 = 56;
int schoolLED = 2;
int familyLED = 4;

//Buttons
int startButton = 28;
int finishButton = 32;
int audioButton1 = 55;
int audioButton2 = 57;
int schoolButton = 3;
int familyButton = 5;

//audio file numbers
byte audio1 = 1;
byte audio2 = 2;
byte family = 3;
byte school = 4;
byte ageAudios[] = {5, 6, 7, 8, 9, 10, 11};

//Potentiometer
int agePin = A14;
long lastAgeTime = 0;

//LED strip variables
int ledStrip = 24;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(76, ledStrip, NEO_GRB + NEO_KHZ800);

long inProgress = false;
long gotData = false;
long startOn = false;
long startPressed = false;
long flashDelay = 400;
long lastFlash = 0;
long startPressEnd = 0;
long lastButtonPress = 0;
long firstAge = true;
long initialise = true;
byte lastIndex = 0;

long startTime = 0;
long endTime = 0;
long duration = 0;

const int resultsLength = 4;
int results[resultsLength];
//results[0] = boxID;
//results[1] = duration
//results[2] = age (0->3, 1->4, 2->5, 3->6, 4->7, 5->8, 6->9+)
//results[3] = with (0->school, 1->family)

void setup(){
  Serial.begin(9600);
  
  //initialise WIRE
  Wire.begin();
  
  //initialise LEDs
  pinMode(startLED, OUTPUT); digitalWrite(startLED, LOW);
  pinMode(finishLED, OUTPUT); digitalWrite(finishLED, LOW);
  pinMode(audioLED1, OUTPUT); digitalWrite(audioLED1, LOW);
  pinMode(audioLED2, OUTPUT); digitalWrite(audioLED2, LOW);
  pinMode(familyLED, OUTPUT); digitalWrite(familyLED, LOW);
  pinMode(schoolLED, OUTPUT); digitalWrite(schoolLED, LOW);
  
  //initialise Buttons
  pinMode(startButton, INPUT); 
  pinMode(finishButton, INPUT); 
  pinMode(audioButton1, INPUT); 
  pinMode(audioButton2, INPUT); 
  pinMode(familyButton, INPUT);
  pinMode(schoolButton, INPUT); 
  
  //initialise LED strip
  strip.begin();
  
  //initialise results
  resetResults();
  
   //begin connections
   firstConnection = true;
  if(connectToWifi()){
    connectToServer();
  }
}

void loop(){
  //ping server
  if(millis() - lastPing > pingDelay){
    pingServer();
    lastPing = millis();
  }
  
  //wait for start to be pushed - flash start button
  if(!inProgress){  //BOX NOT ACTIVATED
    //turn off LED strip
    turnOffStrip();
    
    //turn off audio buttons
    digitalWrite(audioLED1, LOW);
    digitalWrite(audioLED2, LOW);
    
    //turn off Finish button
    digitalWrite(finishLED, LOW);
    
    //turn off answer LEDs
    digitalWrite(familyLED, LOW);
    digitalWrite(schoolLED, LOW);
    
    //check if there is data to upload
    if(gotData){
      uploadResults();
      resetResults();
      gotData = false;
    }
  
    if(digitalRead(startButton) == LOW){  //REALLY HIGH
      if(!startPressed && (millis() - startPressEnd) > 1000){
        startPressed = true;
      }
    }else{
      if(startPressed){
        //logStartTime(); //write start time to log file
        startPressEnd = millis();
        startPressed = false;
        digitalWrite(startLED, LOW);
        startOn = false;
        startTime = millis();
        inProgress = true;
      }else{
        if(startOn){
          if(millis() - lastFlash > flashDelay){
            digitalWrite(startLED, LOW);
            startOn = false;
            lastFlash = millis();
          }
        }else{
          if(millis() - lastFlash > flashDelay){
            digitalWrite(startLED, HIGH);
            startOn = true;
            lastFlash = millis();
          }
        }
      }
    }
    
  }else{  //BOX ACTIVATED
    //turn on LED strip
    turnOnStrip();
    
    //light up audio buttons
    digitalWrite(audioLED1, HIGH);
    digitalWrite(audioLED2, HIGH);
    
    //light up finish button
    digitalWrite(finishLED, HIGH);
    
    //get age
    int val = analogRead(agePin);
    //process val
    byte audioIndex = 0;
    if(val <= 23){     //age 3
      results[2] = 3;
      audioIndex = 0;
    }else if(val > 23 && val <= 221){  //age 4
      results[2] = 4;
      audioIndex = 1;
    }else if(val > 221 && val <= 419){  //age 5
      results[2] = 5;
      audioIndex = 2;
    }else if(val > 419 && val <= 595){  //age 6
      results[2] = 6;
      audioIndex = 3;
    }else if(val > 595 && val <= 785){  //age 7
      results[2] = 7;
      audioIndex = 4;
    }else if(val > 785 && val <= 978){  //age 8
      results[2] = 8;
      audioIndex = 5;
    }else if(val > 978){  //age 9+
      results[2] = 9;
      audioIndex = 6;
    }
    
    if(initialise){
      lastIndex = audioIndex;
      initialise = false;
    }
    
    if((abs(audioIndex-lastIndex) == 1) && (millis() - lastAgeTime > 500)){
      Serial.print("audio index = ");
      Serial.println(audioIndex);
      if(!firstAge){
        Wire.beginTransmission(1);
        Wire.write(ageAudios[audioIndex]);
        Wire.endTransmission();
      }
      lastIndex = audioIndex;
      lastAgeTime = millis();
      firstAge = false;
    }
    
    //check for button presses
    if(digitalRead(audioButton1) == HIGH){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play audio 1
        Wire.beginTransmission(1);
        Wire.write(audio1);
        Wire.endTransmission();
      }
    }
    if(digitalRead(audioButton2) == HIGH){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play audio 2
        Wire.beginTransmission(1);
        Wire.write(audio2);
        Wire.endTransmission();
      }
    }
    if(digitalRead(familyButton) == LOW){  //REALLY means HIGH
      if(millis() - lastButtonPress > 500){
        results[3] = 1;
        digitalWrite(schoolLED, LOW);
        digitalWrite(familyLED, HIGH);
        lastButtonPress = millis();
        Wire.beginTransmission(1);
        Wire.write(family);
        Wire.endTransmission();
      }
    }
    if(digitalRead(schoolButton) == LOW){  //REALLY means HIGH
      if(millis() - lastButtonPress > 500){
        results[3] = 0;
        digitalWrite(familyLED, LOW);
        digitalWrite(schoolLED, HIGH);
        lastButtonPress = millis();
        Wire.beginTransmission(1);
        Wire.write(school);
        Wire.endTransmission();
      }
    }
    if(digitalRead(finishButton) == LOW){  //REALLY means HIGH
      endTime = millis();
      duration = endTime - startTime;
      results[1] = duration;
      gotData = true;
      inProgress = false;
    }
  }
  if(!client.connected()){
    connectToServer();
  }
}

/***************************
HELPER METHODS
***************************/
boolean connectToWifi(){
  Serial.println("Connecting to wifi...");
  if(status != WL_CONNECTED){
    status = WiFi.begin(ssid, password);
  }
  if(status != WL_CONNECTED){
    Serial.print("could not connect to wifi: ");
    Serial.println(ssid);
    return false;
  }else{
    Serial.print("connected to wifi: ");
    Serial.println(ssid);
  }
  return true;
}

boolean connectToServer(){
  Serial.println("Connecting to server...");
  if(!client.connected()){
    //if(!firstConnection){
      //client.flush();
      //client.stop();
    //}
    client.connect(server, port);
    //firstConnection = false;
  }
  if(!client.connected()){
    client.flush();
    client.stop();
    Serial.print("could not connect to server: ");
    Serial.println(server);
    return false;
  }else{
    Serial.print("connected to server: ");
    Serial.println(server);
  }
  return true;
}

void uploadResults(){
  Serial.println("uploading results...");
  if(connectToWifi() && connectToServer()){
    post();
    
    //close server connection
    //client.flush();
    //client.stop();
  }
}

void pingServer(){
  Serial.println("pinging server...");
  if(client.connected()){
    client.println("POST /cubes.html HTTP/1.1");
    client.println("Host: 192.168.1.100");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: keep-alive");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(1);
    client.println();
    client.println("p");
  }else{
    Serial.println("connection failed at ping attempt");
  }
}

void post(){
  String postData = "";
  for(int i=0; i<resultsLength; i++){  //add results
    postData = String(postData+results[i]);
    if(i!=resultsLength-1){
      postData = postData+",";
    }
  }
  Serial.println(postData);
  if(client.connected()){
    client.println("POST /cubes.html HTTP/1.1");
    client.println("Host: icri-nodejs.cs.ucl.ac.uk");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: keep-alive");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.println(postData);
  }else{
    Serial.println("connection failed at data upload");
  }
}

void turnOnStrip(){
  for(int i=0; i<strip.numPixels(); i++){
    if(i%2 == 0){
      strip.setPixelColor(i, strip.Color(0,255,0));  //GREEN
    }
  }
  strip.show();
}
  
void turnOffStrip(){
  for(int i=0; i<strip.numPixels(); i++){
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}

void resetResults(){
  for(int i=0; i<resultsLength; i++){
    results[i] = -1;
  }
  results[0] = boxID;
}

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  while(1);
}
