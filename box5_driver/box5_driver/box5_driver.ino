#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//box variables
const int boxID = 5;

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
int startLED = 2;
int finishLED = 26;
int videoLED1 = 3;
int videoLED2 = 4;
int videoLED3 = 5;

//Buttons
int startButton = 57;
int finishButton = 28;
int videoButton1 = 56;
int videoButton2 = 55;
int videoButton3 = 54;

String videoCommands[] = {"play2", "play3", "play4", "play5"};
String random1;
String random2;
long initialiseVideo = true;

//TX = xx
//RX = xx

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

const int resultsLength = 2;
int results[resultsLength];
//results[0] = boxID;
//results[1] = duration

void setup(){
  Serial.begin(9600);
  
  //initialise LEDs
  pinMode(startLED, OUTPUT); digitalWrite(startLED, LOW);
  pinMode(finishLED, OUTPUT); digitalWrite(finishLED, LOW);
  pinMode(videoLED1, OUTPUT); digitalWrite(videoLED1, LOW);
  pinMode(videoLED2, OUTPUT); digitalWrite(videoLED2, LOW);
  pinMode(videoLED3, OUTPUT); digitalWrite(videoLED3, LOW);
  
  //initialise Buttons
  pinMode(startButton, INPUT); 
  pinMode(finishButton, INPUT); 
  pinMode(videoButton1, INPUT); 
  pinMode(videoButton2, INPUT); 
  pinMode(videoButton3, INPUT);
  
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

//void loop(){
//  uploadResults();
//  while(1){}
//}

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
    
    //turn off video buttons
    digitalWrite(videoLED1, LOW);
    digitalWrite(videoLED2, LOW);
    digitalWrite(videoLED3, LOW);
    
    //turn off Finish button
    digitalWrite(finishLED, LOW);
    
    initialiseVideo = true;
    
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
    digitalWrite(videoLED1, HIGH);
    digitalWrite(videoLED2, HIGH);
    digitalWrite(videoLED3, HIGH);
    
    //light up finish button
    digitalWrite(finishLED, HIGH);
    
    if(initialiseVideo){
      setRandoms();
      initialiseVideo = false;
    }
    
    //check for button presses
    if(digitalRead(videoButton1) == LOW){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play video 1
         Serial.println("play1");
      }
    }
    if(digitalRead(videoButton2) == LOW){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play video 2
        Serial.println(random1);
      }
    }
    if(digitalRead(videoButton3) == LOW){
      if(millis() - lastButtonPress > 500){
        lastButtonPress = millis();
        //play video 3
        Serial.println(random2);
      }
    }
    if(digitalRead(finishButton) == LOW){  //REALLY means HIGH
      Serial.println("reset");
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
  //Serial.println("Connecting to wifi...");
  if(status != WL_CONNECTED){
    status = WiFi.begin(ssid, password);
  }
  if(status != WL_CONNECTED){
    //Serial.print("could not connect to wifi: ");
    //Serial.println(ssid);
    return false;
  }//else{
    //Serial.print("connected to wifi: ");
    //Serial.println(ssid);
  //}
  return true;
}

boolean connectToServer(){
  //Serial.println("Connecting to server...");
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
    //Serial.print("could not connect to server: ");
    //Serial.println(server);
    return false;
  }//else{
    //Serial.print("connected to server: ");
    //Serial.println(server);
  //}
  return true;
}

void uploadResults(){
  //Serial.println("uploading results...");
  if(connectToWifi() && connectToServer()){
    post();
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
  //Serial.println(postData);
  if(client.connected()){
    client.println("POST /cubes.html HTTP/1.1");
    client.println("Host: icri-nodejs.cs.ucl.ac.uk");
    client.println("User-Agent: Arduino/1.0");
    client.println("Connection: close");
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.print("Content-Length: ");
    client.println(postData.length());
    client.println();
    client.println(postData);
  }//else{
    //Serial.println("connection failed at data upload");
  //}
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

void setRandoms(){
  int randIndex1 = random(0, 4);
  int randIndex2 = random(0, 4);
  while(randIndex1 == randIndex2){
    randIndex2 = random(0, 4);
  }
  random1 = videoCommands[randIndex1];
  random2 = videoCommands[randIndex2]; 
}

void error(char *str)
{
  //Serial.print("error: ");
  //Serial.println(str);
  while(1);
}
