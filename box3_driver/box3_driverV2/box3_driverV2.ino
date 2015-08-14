#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//box variables
const int boxID = 3;

//WIFI variables
char ssid[] = "VodafoneMobileWiFi-3BD572";
char password[] = "0561993356";
int status = WL_IDLE_STATUS;

//server variables
WiFiClient client;
char server[] = "icri-nodejs.cs.ucl.ac.uk";
int port = 6543;

const int responseLength = 78;
char response[responseLength];
int index = 0;

//long firstConnection;
//long pingDelay = 10000;
//long lastPing = 0;

//LEDs
int startLED = 26;
int finishLED = 30;
int audioLED = 34;
int locationLEDs[] = {2, 4, 54, 56, 38, 42};

//Buttons
int startButton = 28;
int finishButton = 32;
int audioButton = 36;
int locationButtons[] = {3, 5, 55, 57, 40, 44};

//audio file numbers
byte audio = 1;
byte locationAudios[] = {2, 3, 4, 5, 6, 7};

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

long startTime = 0;
long endTime = 0;
long duration = 0;

const int resultsLength = 3;
int results[resultsLength];
//results[0] = boxID;
//results[1] = duration
//results[2] = location (1->loc1, 2->loc2, 3->loc3, 4->loc4, 5->loc5, 6->loc6)

void setup(){
  Serial.begin(9600);
  
  //initialise WIRE
  Wire.begin();
  
  //initialise LEDs
  pinMode(startLED, OUTPUT); digitalWrite(startLED, LOW);
  pinMode(finishLED, OUTPUT); digitalWrite(finishLED, LOW);
  pinMode(audioLED, OUTPUT); digitalWrite(audioLED, LOW);
  for(int i=0; i<6; i++){
    pinMode(locationLEDs[i], OUTPUT); digitalWrite(locationLEDs[i], LOW);
  }
  
  //initialise Buttons
  pinMode(startButton, INPUT); 
  pinMode(finishButton, INPUT); 
  pinMode(audioButton, INPUT); 
  for(int i=0; i<6; i++){
    pinMode(locationButtons[i], INPUT); 
  }
  
  //initialise LED strip
  strip.begin();
  
  //initialise results
  resetResults();
  
   //begin connections
   //connectToWifi();
   //firstConnection = true;
  //if(connectToWifi()){
    //connectToServer();
  //}
  if(!connectToWifi()){
    error("no wifi connection");
  }
}

void loop(){
  //ping server
//  if(millis() - lastPing > pingDelay){
//    pingServer();
//    lastPing = millis();
//  }
  
  //wait for start to be pushed - flash start button
  if(!inProgress){  //BOX NOT ACTIVATED
    //turn off LED strip
    turnOffStrip();
    
    //turn off audio buttons
    digitalWrite(audioLED, LOW);
    
    //turn off Finish button
    digitalWrite(finishLED, LOW);
    
    //turn off answer LEDs
    for(int i=0; i<6; i++){
      digitalWrite(locationLEDs[i], LOW);
    }
    
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
    digitalWrite(audioLED, HIGH);
    
    //light up finish button
    digitalWrite(finishLED, HIGH);
    
    //check for button presses
    if(digitalRead(audioButton) == HIGH){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play audio 1
        Wire.beginTransmission(1);
        Wire.write(audio);
        Wire.endTransmission();
      }
    }
    for(int i=0; i<6; i++){
      if(digitalRead(locationButtons[i]) == LOW){
        if(millis() - lastButtonPress > 500){
          lastButtonPress = millis();
          results[2] = i+1;
          turnOnLED(i);
          //play location 1
          Wire.beginTransmission(1);
          Wire.write(locationAudios[i]);
          Wire.endTransmission();
        }
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
  //if(!client.connected()){
    //connectToServer();
  //}
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

//boolean connectToServer(){
//  Serial.println("Connecting to server...");
//  if(!client.connected()){
//    //if(!firstConnection){
//      //client.flush();
//      //client.stop();
//    //}
//    client.connect(server, port);
//    //firstConnection = false;
//  }
//  if(!client.connected()){
//    client.flush();
//    client.stop();
//    Serial.print("could not connect to server: ");
//    Serial.println(server);
//    return false;
//  }else{
//    Serial.print("connected to server: ");
//    Serial.println(server);
//  }
//  return true;
//}
//
//void uploadResults(){
//  Serial.println("uploading results...");
//  if(connectToWifi() && connectToServer()){
//    post();
//    
//    //close server connection
//    //client.flush();
//    //client.stop();
//  }
//}
//
//void pingServer(){
//  Serial.println("pinging server...");
//  if(client.connected()){
//    client.println("POST /cubes.html HTTP/1.1");
//    client.println("Host: 192.168.1.100");
//    client.println("User-Agent: Arduino/1.0");
//    client.println("Connection: keep-alive");
//    client.println("Content-Type: application/x-www-form-urlencoded");
//    client.print("Content-Length: ");
//    client.println(1);
//    client.println();
//    client.println("p");
//  }else{
//    Serial.println("connection failed at ping attempt");
//  }
//}

void uploadResults(){
  String postData = "";
  for(int i=0; i<resultsLength; i++){  //add results
    postData = String(postData+results[i]);
    if(i!=resultsLength-1){
      postData = postData+",";
    }
  }
  Serial.println(postData);
  
  Serial.println("Opening connection with server...");
  if(client.connect(server, port)){
    Serial.println("connected");
    
    Serial.println("sending data...");
  
    //Make a HTTP request:
    client.println("POST /smalltalkUpdate.html HTTP/1.1");
    client.println("Host: icri-nodejs.cs.ucl.ac.uk");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: keep-alive");
    client.println("Content-Type: text/html");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.println(postData);
    
    //listen for response
    while(index < responseLength){
      if(client.available()){
        char c = client.read();
        response[index++] = c;
      }
    }
    
    client.stop();
    Serial.println(response);
    Serial.println("server connection closed");
    
    //clear response
    memset(response, 0, responseLength);
    index = 0;
  }else{
    client.stop();
    Serial.println("could not open connection with server!");
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

void turnOnLED(int index){
  for(int i=0; i<6; i++){
    digitalWrite(locationLEDs[i], LOW);
  }
  digitalWrite(locationLEDs[index], HIGH);
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
