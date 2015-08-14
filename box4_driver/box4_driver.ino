#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_NeoPixel.h>

//box variables
const int boxID = 4;

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
int startLED = 38;
int finishLED = 42;
int audioLED = 5;
int animalLEDs[] = {26, 28, 30, 32, 34, 36};

//Buttons
int startButton = 40;
int finishButton = 44;
int audioButton = 6;

//Rotary switch
int switchpos[6] = {A0, A1, A2, A3, A4, A5};
int prevpos;
int firsttime = 1;

//audio file numbers
byte audio = 1;
byte animalAudios[] = {2, 3, 4, 5, 6, 7};

//LED strip variables
int ledStrip = 4;
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
//results[2] = animal (1->animal1, 2->animal2, 3->animal3, 4->animal4, 5->animal5, 6->animal6)

void setup(){
  Serial.begin(9600);
  
  //initialise WIRE
  Wire.begin();
  
  //initialise LEDs
  pinMode(startLED, OUTPUT); digitalWrite(startLED, LOW);
  pinMode(finishLED, OUTPUT); digitalWrite(finishLED, LOW);
  pinMode(audioLED, OUTPUT); digitalWrite(audioLED, LOW);
  for(int i=0; i<6; i++){
    pinMode(animalLEDs[i], OUTPUT); digitalWrite(animalLEDs[i], LOW);
  }
  
   //initialise rotary switch
  for (int i=0; i<6; i++)
  {
    pinMode(switchpos[i], INPUT);
  }
  
  //initialise Buttons
  pinMode(startButton, INPUT); 
  pinMode(finishButton, INPUT); 
  pinMode(audioButton, INPUT);
  
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
    firsttime = 1;
    prevpos = -1;
    
    //turn off audio buttons
    digitalWrite(audioLED, LOW);
    
    //turn off Finish button
    digitalWrite(finishLED, LOW);
    
    //turn off answer LEDs
    for(int i=0; i<6; i++){
      digitalWrite(animalLEDs[i], LOW);
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
    
    //get rotary switch value - if new value, play audio
    for (int i=0; i<6; i++)
    {
      if (digitalRead(switchpos[i]) == HIGH)
      {
          if (i != prevpos)
          {
            Serial.print(i);
            Serial.println(" position high");
            digitalWrite(animalLEDs[i], HIGH);
            Serial.println(firsttime);
            if (firsttime == 0)
            {
              Wire.beginTransmission(1);
              Wire.write(animalAudios[i]);
              Wire.endTransmission();
            }
            firsttime = 0;
            digitalWrite(animalLEDs[prevpos], LOW);
            prevpos = i;
          }          
          results[2] = i+1;
      }
    }
    
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
    client.println("Connection: close");
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

void turnOnLED(int index){
  for(int i=0; i<6; i++){
    digitalWrite(animalLEDs[i], LOW);
  }
  digitalWrite(animalLEDs[index], HIGH);
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
