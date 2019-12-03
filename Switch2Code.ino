#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads; //(0x48);

SoftwareSerial btSerial(2, 3); // RX, TX

const int senderPin = 7; //Tells us we are sending over Serial
const int receiverPin = 8; //Tells us we are receiving confirmation over Serial
const int sendSwitch = 2;

int count = 0;
int flag2 = 0;
int flag3 = 0;
int current_flag_2 = 0;
int current_flag_3 = 0;
char data[5] = {};

const float FACTOR = 30; //CT Calibration factors
const float multiplier = 0.0625F;

float degreesC;
unsigned long timer2 = 0;
unsigned long timer3 = 0;

int buttonState;             // the current reading from the input pin
int buttonState1;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int lastButtonState1 = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTime1 = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  //Pin Modes
  pinMode(senderPin, OUTPUT);
  pinMode(receiverPin, OUTPUT);
  pinMode(sendSwitch, INPUT_PULLUP);
  pinMode(8, OUTPUT);
  pinMode(9, OUTPUT);
  pinMode(6, INPUT_PULLUP);
  pinMode(7, INPUT_PULLUP);

  ads.setGain(GAIN_TWO); // +/- 2.048V 1 bit = 0.0624mV
  ads.begin();
  
  //Setup and flush the serials to begin
  btSerial.begin(9600);
  Serial.begin(9600);
  btSerial.flush();
  Serial.flush();
  delay(50);
  digitalWrite(8, HIGH);
  digitalWrite(9, HIGH);
  delay(50);
  digitalWrite(8, LOW);
  digitalWrite(9, LOW);
}

void loop() {
  getPower2();
  if ((timer2 == 0 || millis() >= timer2) && current_flag_2 == 1){
    float power2 = getPower2();
    String sendPower2 = "<" + String(power2, 3) + ">";
    char charArray2[sendPower2.length() + 1]; //Convert to byte array
    sendPower2.toCharArray(charArray2, sendPower2.length()+1);
    btSerial.write(charArray2);
    current_flag_2 = 0;
    //Reset the timer for another 1 seconds.
    timer2 = millis() + 1000;
      
  }
  if ((timer3 == 0 || millis() >= timer3) && current_flag_3 == 1){
    float power3 = getPower3();
    String sendPower3 = "<" + String(power3, 3) + ">";
    char charArray3[sendPower3.length() + 1]; //Convert to byte array
    sendPower3.toCharArray(charArray3, sendPower3.length()+1);
    btSerial.write(charArray3);
    current_flag_3 = 0;
    //Reset the timer for another 1 seconds.
    timer3 = millis() + 1000;
      
  }
//Debouncing
  int reading = digitalRead(6);
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the LED if the new button state is HIGH
      if (buttonState == LOW) {
        if(flag2){
          digitalWrite(8, LOW);
          flag2 = 0;
        }
        else{
          digitalWrite(8, HIGH);
          flag2 = 1;
        }
      }
    }
  }
  int reading1 = digitalRead(7);
  if (reading1 != lastButtonState1) {
    // reset the debouncing timer
    lastDebounceTime1 = millis();
  }
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading != buttonState1) {
      buttonState1 = reading1;
      if (buttonState1 == LOW) {
        if(flag3){
          digitalWrite(9, LOW);
          flag3 = 0;
        }
        else{
          digitalWrite(9, HIGH);
          flag3 = 1;
        }
      }
    }
  }
  lastButtonState = reading;
  lastButtonState1 = reading1;
  
//Bluetooth receive
  while (btSerial.available()) {
    char rpiMessage = btSerial.read();
    Serial.print(rpiMessage);
    if(rpiMessage == '<'){
      count = 0;
      for (int x = 0; x < sizeof(data) / sizeof(data[0]); x++)
      {
        data[x] = 0;
      }
      data[count] = rpiMessage;
    }
    else if(rpiMessage == '>'){
      data[count] = rpiMessage;
      if(data[1] == '2'){
        if(data[3] == '1'){
          digitalWrite(8, HIGH);
          flag2 = 1;
        }
        else if(data[3] == '0'){
          digitalWrite(8, LOW);
          flag2 = 0;
        }
        else if(data[3] == '2'){
          current_flag_2 = 1;
        }
      }
      if(data[1] == '3'){
        if(data[3] == '1'){
          digitalWrite(9, HIGH);
          flag3 = 1;
        }
        else if(data[3] == '0'){
          digitalWrite(9, LOW);
          flag3 = 0;
        }
        else if(data[3] == '2'){
          current_flag_3 = 1;
        }
      }
        
    }
    else{
      data[count] = rpiMessage;
    }
    count ++;
  } 
}

float getPower2(){
  float voltage;
  float current;
  float sum = 0;
  long Time = millis();
  int counter = 0;

  while(millis() - Time < 1000){
    voltage = ads.readADC_Differential_0_1() * multiplier;
    current = voltage * FACTOR; //Conductance
    current /= 1000.0;

    sum += sq(current);
    counter = counter + 1;
  }
  current = sqrt(sum/counter); //avg
  float currentRMS = current;
  float power = 120 * currentRMS;
  Serial.println(power);
  return(power);
}

float getPower3() {
  float voltage;
  float current;
  float sum = 0;
  long Time = millis();
  int counter = 0;

  while(millis() - Time < 1000){
    voltage = ads.readADC_Differential_2_3() * multiplier;
    current = voltage * FACTOR; //Conductance
    current /= 1000.0;

    sum += sq(current);
    counter = counter + 1;
  }
  current = sqrt(sum/counter); //avg
  float currentRMS = current;
  float power = 120 * currentRMS;
  //Serial.println(power);
  return(power);
}

