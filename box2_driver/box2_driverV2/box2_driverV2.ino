#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//box variables
const int boxID = 2;

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
int audioLED1 = 54;
int audioLED2 = 56;
int yesLED = 2;
int noLED = 4;

//Buttons
int startButton = 28;
int finishButton = 32;
int audioButton1 = 55;
int audioButton2 = 57;
int yesButton = 3;
int noButton = 5;

//audio file numbers
byte audio1 = 1;
byte audio2 = 2;
byte yes = 3;
byte no = 4;
byte againAudios[] = {7, 6, 5};

//Potentiometer
int againPin = A14;
long lastAgainTime = 0;

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
long firstAgain = true;
long initialise = true;
byte lastIndex = 0;

long startTime = 0;
long endTime = 0;
long duration = 0;

const int resultsLength = 4;
int results[resultsLength];
//results[0] = boxID;
//results[1] = duration
//results[2] = before (0->yes, 1->no)
//results[3] = again (0->no, 1->maybe, 2->yes)

void setup(){
  Serial.begin(9600);
  
  //initialise WIRE
  Wire.begin();
  
  //initialise LEDs
  pinMode(startLED, OUTPUT); digitalWrite(startLED, LOW);
  pinMode(finishLED, OUTPUT); digitalWrite(finishLED, LOW);
  pinMode(audioLED1, OUTPUT); digitalWrite(audioLED1, LOW);
  pinMode(audioLED2, OUTPUT); digitalWrite(audioLED2, LOW);
  pinMode(yesLED, OUTPUT); digitalWrite(yesLED, LOW);
  pinMode(noLED, OUTPUT); digitalWrite(noLED, LOW);
  
  //initialise Buttons
  pinMode(startButton, INPUT); 
  pinMode(finishButton, INPUT); 
  pinMode(audioButton1, INPUT); 
  pinMode(audioButton2, INPUT); 
  pinMode(yesButton, INPUT);
  pinMode(noButton, INPUT); 
  
  //initialise LED strip
  strip.begin();
  
  //initialise results
  resetResults();
  
   //begin connections
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
    digitalWrite(audioLED1, LOW);
    digitalWrite(audioLED2, LOW);
    
    //turn off Finish button
    digitalWrite(finishLED, LOW);
    
    //turn off answer LEDs
    digitalWrite(yesLED, LOW);
    digitalWrite(noLED, LOW);
    
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
    
    //get again pin
    int val = analogRead(againPin);
    //process val
    byte audioIndex = 0;
    if(val <= 325){  //NO
      results[3] = 0;
      //audioIndex = 0;
    }else if(val > 325 && val <= 794){  //MAYBE
      results[3] = 1;
      //audioIndex = 1;
    }else if(val > 794){  //YES
      results[3] = 2;
      //audioIndex = 2;
    }
    
//    if(initialise){
//      lastIndex = audioIndex;
//      initialise = false;
//    }
//    
//    if((abs(audioIndex-lastIndex) == 1) && (millis() - lastAgainTime > 500)){
//      if(!firstAgain){
//        Wire.beginTransmission(1);
//        Wire.write(againAudios[audioIndex]);
//        Wire.endTransmission();
//      }
//      lastIndex = audioIndex;
//      lastAgainTime = millis();
//      firstAgain = false;
//    }
    
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
    if(digitalRead(yesButton) == LOW){  //REALLY means HIGH
      if(millis() - lastButtonPress > 500){
        results[2] = 0;
        digitalWrite(noLED, LOW);
        digitalWrite(yesLED, HIGH);
        lastButtonPress = millis();
        Wire.beginTransmission(1);
        Wire.write(yes);
        Wire.endTransmission();
      }
    }
    if(digitalRead(noButton) == LOW){  //REALLY means HIGH
      if(millis() - lastButtonPress > 500){
        results[2] = 1;
        digitalWrite(yesLED, LOW);
        digitalWrite(noLED, HIGH);
        lastButtonPress = millis();
        Wire.beginTransmission(1);
        Wire.write(no);
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
