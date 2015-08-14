#include <Adafruit_NeoPixel.h>
#include <Wire.h>

//slave variables
int id = 1;
const int dataLength = 5;
char data[dataLength];
long isActive = false;

//LED strip variables
int ledStrip = 19;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(60, ledStrip, NEO_GRB + NEO_KHZ800);

//pin assignments
int submitButton = A1;
long submitPressed = false;

int agePin = A13;
char ageAnswer = 'X';

int fromOptions = 4; //should be 4
int fromButtonPins[] = {A2,A3,A4,A5};
int fromLedPins[] = {18,17,16,15};
char fromAnswer = 'X';

int withOptions = 4; //should be 4
int withButtonPins[] = {A6,A7,A8,A9};
int withLedPins[] = {14,2,3,4};
char withAnswer = 'X';

int genderOptions = 3;
int genderButtonPins[] = {A10,A11,A12};
int genderLedPins[] = {5,6,7};
char genderAnswer = 'X';

void setup(){
  
  //setup LED strip
  strip.begin();
  
  //turnOnStrip();
 
  //setup submit button
  pinMode(submitButton, INPUT);
  digitalWrite(submitButton, LOW);
  
  //setup FROM buttons and leds
  for(int i=0; i<fromOptions; i++){
    pinMode(fromButtonPins[i], INPUT);
    digitalWrite(fromButtonPins[i], LOW);
    pinMode(fromLedPins[i], OUTPUT);
    digitalWrite(fromLedPins[i], HIGH);
  }
  
  //setup WITH buttons
  for(int i=0; i<withOptions; i++){
    pinMode(withButtonPins[i], INPUT);
    digitalWrite(withButtonPins[i], LOW);
    pinMode(withLedPins[i], OUTPUT);
    digitalWrite(withLedPins[i], HIGH);
  }
  
  //setup GENDER buttons
  for(int i=0; i<genderOptions; i++){
    pinMode(genderButtonPins[i], INPUT);
    digitalWrite(genderButtonPins[i], LOW);
    pinMode(genderLedPins[i], OUTPUT);
    digitalWrite(genderLedPins[i], HIGH);
  }
  
  Wire.begin(id);
  Wire.onReceive(handleEvent);
  Wire.onRequest(handleRequest);
  
  Serial.begin(9600);
  Serial.print("slave initialised with ID: ");
  Serial.println(id);
}

void loop(){
  if(isActive){
    
    //turn on LED strip
    turnOnStrip();
    
    //check for submit press
    if(digitalRead(submitButton) == HIGH){
      submitPressed = true;
    }
    
    //get age
    int val = analogRead(agePin);
    if(val > 1020){
      ageAnswer = 5+'0';
    }else if(val <= 1020 && val > 805){
      ageAnswer = 4+'0';
    }else if(val <= 805 && val > 499){
      ageAnswer = 3+'0';
    }else if(val <= 499 && val > 312){
      ageAnswer = 2+'0';
    }else if(val <=312 && val > 52){
      ageAnswer = 1+'0';
    }else{
      ageAnswer = 0+'0';
    }
    
    //check for FROM presses
    for(int i=0; i<fromOptions; i++){
      if(digitalRead(fromButtonPins[i]) == HIGH){
        //turn them all off first
        for(int j=0; j<fromOptions; j++){
          digitalWrite(fromLedPins[j], HIGH);
        }
        //turn on selected LED
        digitalWrite(fromLedPins[i], LOW);
        fromAnswer = i+'0';
      }
    }
    
    //check for WITH presses
    for(int i=0; i<withOptions; i++){
      if(digitalRead(withButtonPins[i]) == HIGH){
        //turn them all off first
        for(int j=0; j<withOptions; j++){
          digitalWrite(withLedPins[j], HIGH);
        }
        //turn on selected LED
        digitalWrite(withLedPins[i], LOW);
        withAnswer = i+'0';
      }
    }
    
    //check for GENDER presses
    for(int i=0; i<genderOptions; i++){
      if(digitalRead(genderButtonPins[i]) == HIGH){
        //turn them all off first
        for(int j=0; j<genderOptions; j++){
          digitalWrite(genderLedPins[j], HIGH);
        }
        //turn on selected LED
        digitalWrite(genderLedPins[i], LOW);
        genderAnswer = i+'0';        
      }
    }
  }else{
    submitPressed = false;
    
    //turn off LED strip
    turnOffStrip();
  
    //turn off all LEDS
    for(int i=0; i<fromOptions; i++){
      digitalWrite(fromLedPins[i], HIGH);
      fromAnswer = 'X';
    }
    for(int i=0; i<withOptions; i++){
      digitalWrite(withLedPins[i], HIGH);
      withAnswer = 'X';
    }
    for(int i=0; i<genderOptions; i++){
      digitalWrite(genderLedPins[i], HIGH);
      genderAnswer = 'X';
    }

    //reset data
    memset(data, 0, dataLength);
  }
  
  delay(100);
}

void readInputs(){
  //get data from input devices
  data[1] = ageAnswer;
  data[2] = fromAnswer;
  data[3] = withAnswer;
  data[4] = genderAnswer;
}

void handleEvent(int num){
  char event = Wire.read();
  
  Serial.print("Event received:");
  Serial.println(event);
  
  if(event == 'R'){  //RESET
    Serial.println("resetting slave...");
    isActive = false;
  }
}

void handleRequest(){
  Serial.println("Data request received...");
  
  isActive = true;
  
  if(submitPressed){
    //get data
    readInputs();
    Serial.println("returning Y with data");
    data[0] = 'Y';
    Wire.write(data);
    isActive = false;
  }else{
    Serial.println("returning N with null data");
    data[0] = 'N';
    Wire.write(data);
  }
}


/***************************
HELPER METHODS
***************************/
void turnOnStrip(){
  for(int i=0; i<strip.numPixels(); i++){
    strip.setPixelColor(i, strip.Color(0,255,0));  //GREEN
  }
  strip.show();
}
  
void turnOffStrip(){
  for(int i=0; i<strip.numPixels(); i++){
    strip.setPixelColor(i, strip.Color(0,0,0));
  }
  strip.show();
}
