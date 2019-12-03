#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>

Adafruit_ADS1115 ads; //(0x48);

SoftwareSerial btSerial(14, 16); // RX, TX

const int senderPin = 7; //Tells us we are sending over Serial
const int receiverPin = 8; //Tells us we are receiving confirmation over Serial
const int sendSwitch = 2;

int count = 0;
int flag = 0;
int current_flag = 0;
char data[5] = "";

const float FACTOR = 30; //CT Calibration factors
const float multiplier = 0.0625F;

float degreesC;
unsigned long timer = 0;
int sendStatus = 0;

int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() {
  //Pin Modes
  pinMode(senderPin, OUTPUT);
  pinMode(receiverPin, OUTPUT);
  pinMode(sendSwitch, INPUT_PULLUP);
  pinMode(7, OUTPUT);
  pinMode(6, INPUT_PULLUP);

  ads.setGain(GAIN_TWO); // +/- 2.048V 1 bit = 0.0624mV
  ads.begin();
  
  //Setup and flush the serials to begin
  btSerial.begin(9600);
  Serial.begin(9600);
  btSerial.flush();
  Serial.flush();
  delay(50);
  digitalWrite(7, HIGH);
  delay(50);
  digitalWrite(7, LOW);
}

void printMeasure(String prefix, float value, String postfix)
{
  Serial.print(prefix);
  Serial.print(value, 8); // 3 decimals??
  Serial.print(postfix);
}

void loop() {
  int p = getPower();
  //I want to send the current, I want the Raspberry PI to grab it, process it
  //and send back a message. I don't want to continue spamming the raspberry pi, so
  //I will only send a signal every 1 seconds.
  
  //Non-blocking every 10 seconds.
  if ((timer == 0 || millis() >= timer) && current_flag == 1){
    float power = getPower();
    printMeasure("Power: ", power, "W \n");  
    //Send the current power usage. Didn't use readline to avoid blocking problem
    String sendPower = "<" + String(power, 3) + ">";
    
    //Convert to byte array
    char charArray[sendPower.length() + 1];
    sendPower.toCharArray(charArray, sendPower.length()+1);
    
    btSerial.write(charArray);
    current_flag = 0;
    
    //Reset the timer for another 1 seconds.
    timer = millis() + 1000;
      
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
      if (buttonState == LOW) {
        if(flag){
          digitalWrite(7, LOW);
          flag = 0;
        }
        else{
          digitalWrite(7, HIGH);
          flag = 1;
        }
      }
    }
  }
  lastButtonState = reading;

//Bluetooth receive
  while (btSerial.available()) {
    char rpiMessage = btSerial.read();
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
      if(data[1] == '1'){
        if(data[3] == '1'){
          digitalWrite(7, HIGH);
          flag = 1;
        }
        else if(data[3] == '0'){
          digitalWrite(7, LOW);
          flag = 0;
        }
        else if(data[3] == '2'){
          current_flag = 1;
        }
      } 
    }
    else{
      data[count] = rpiMessage;
    }
    count ++;
  }
    
}

float getPower() {
  float voltage;
  float current;
  float sum = 0;
  long Time = millis();
  int counter = 0;

  while(millis() - Time < 1000){
    voltage = ads.readADC_Differential_0_1() * multiplier;
    Serial.print(voltage);
    current = voltage * FACTOR; //Conductance
    current /= 1000.0;

    sum += sq(current);
    counter = counter + 1;
  }
  current = sqrt(sum/counter); //avg
  float currentRMS = current;
  float power = 120 * currentRMS;
  
  //return(power);
  return 10;
}

