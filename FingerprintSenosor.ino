#include <DFRobot_ID809.h>
#define COLLECT_NUMBER 3
#define IRQ         6  

#include <Servo.h>
#define ServoPin 5
Servo servo;
//0 mean closed; 1 means open
int Door=1;

int Button=0;
int ButtonPin=4;

#include <SoftwareSerial.h>
SoftwareSerial Serial1(2, 3);
#define FPSerial Serial1

DFRobot_ID809 fingerprint;

void setup(){
  servo.attach(ServoPin);
  OpenDoor();

  pinMode(ButtonPin,INPUT_PULLUP);
  
  Serial.begin(9600);
  FPSerial.begin(115200);
  
  fingerprint.begin(FPSerial);
  while(!Serial);
  while(fingerprint.isConnected() == false){
    Serial.println("Communication with device failed, please check connection");
    delay(1000);
  }
}

//Blue LED Comparison mode  Yellow LED Registration mode  Red Deletion mode 
void loop(){
  
  if(digitalRead(IRQ)){
    Button=digitalRead(ButtonPin);
    uint16_t i = 0;                   /*timeout*/
    if((fingerprint.collectionFingerprint(5)) != ERR_ID809){
      /*Get the time finger pressed down*/
      /*Set fingerprint LED ring mode, color, and number of blinks 
        Can be set as follows:
        Parameter 1:<LEDMode>
        eBreathing   eFastBlink   eKeepsOn    eNormalClose
        eFadeIn      eFadeOut     eSlowBlink   
        Paramerer 2:<LEDColor>
        eLEDGreen  eLEDRed      eLEDYellow   eLEDBlue
        eLEDCyan   eLEDMagenta  eLEDWhite
        Parameter 3:<number of blinks> 0 represents blinking all the time
        This parameter will only be valid in mode eBreathing, eFastBlink, eSlowBlink */
                          /*LEDMode,LEDColor,blinkCount*/
      fingerprint.ctrlLED(fingerprint.eFastBlink,fingerprint.eLEDBlue,3);  //blue LED blinks quickly 3 times, means it's in fingerprint comparison mode now
      /*Wait for finger to relase */
      while(fingerprint.detectFinger()){
        delay(50);
        i++;
        if(i == 15){             //Yellow LED blinks quickly 3 times, means it's in fingerprint regisrtation mode now
          /*Set fingerprint LED ring to always ON in yellow*/
          fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDYellow, /*blinkCount = */3);
        }else if(i == 30){      //Red LED blinks quickly 3 times, means it's in fingerprint deletion mode now 
          /*Set fingerprint LED ring to always ON in red*/
          fingerprint.ctrlLED(/*LEDMode = */fingerprint.eFastBlink, /*LEDColor = */fingerprint.eLEDRed, /*blinkCount = */3);
        }
      }
    }
    if(i == 0){
      /*Fingerprint capturing failed*/
    }else if(i > 0 && i < 15){
      Serial.println("Enter fingerprint comparison mode");
      /*Compare fingerprints*/
      fingerprintMatching();
    }else if((i >= 15 && i < 30)&&(Button==1)){
      Serial.println("Enter fingerprint registration mode");
      /*Registrate fingerprint*/
      fingerprintRegistration();
    }else if (Button == 1){
      Serial.println("Enter fingerprint deletion mode");
      /*Delete this fingerprint*/
      fingerprintDeletion();
    }
  }else if (digitalRead(ButtonPin)==0){
    RunDoor();
    Serial.println();
    delay(2000);
  }
}
void OpenDoor(){
  Door=1;
  servo.write(45);
 }
void CloseDoor(){
  Door=0;
  servo.write(140);
 }
void RunDoor(){
  if (Door == 1){
    CloseDoor();
  }else if (Door == 0){
    OpenDoor();  
  }
}
//Compare fingerprints
void fingerprintMatching(){
  uint8_t ret = fingerprint.search();
  if(ret != 0){
    fingerprint.ctrlLED(fingerprint.eKeepsOn,fingerprint.eLEDGreen,0);
    Serial.print("Successfully matched,ID=");
    Serial.println(ret);
    RunDoor();
  }else{
    fingerprint.ctrlLED(fingerprint.eKeepsOn,fingerprint.eLEDRed,0);
    Serial.println("Matching failed");
  }
  delay(1000);
  fingerprint.ctrlLED(fingerprint.eNormalClose,fingerprint.eLEDBlue,0);
  Serial.println("-----------------------------");
}

//Fingerprint Registration
void fingerprintRegistration(){
  uint8_t ID,i;
  fingerprint.search();       //Can add "if else" statement to judge whether the fingerprint has been registered. 
  if((ID = fingerprint.getEmptyID()) == ERR_ID809){
    while(1){
      delay(1000);
    }
  }
  Serial.print("Unregistered ID,ID=");
  Serial.println(ID);
  i = 0;
  while(i < COLLECT_NUMBER){
    fingerprint.ctrlLED(fingerprint.eBreathing,fingerprint.eLEDBlue,0);
    Serial.print("The fingerprint sampling of the");
    Serial.print(i+1);
    Serial.println("(th) time is being taken");
    Serial.println("Please press down your finger");
    if((fingerprint.collectionFingerprint(10)) != ERR_ID809){
      fingerprint.ctrlLED(fingerprint.eFastBlink,fingerprint.eLEDYellow,3);
      Serial.println("Capturing succeeds");
      i++;
    }else{
      Serial.println("Capturing fails");
    }
    Serial.println("Please release your finger");
    while(fingerprint.detectFinger());
  }
  if(fingerprint.storeFingerprint(ID) != ERR_ID809){
    Serial.print("Saving succeed，ID=");
    Serial.println(ID);
    fingerprint.ctrlLED(fingerprint.eKeepsOn,fingerprint.eLEDGreen,0);
    delay(1000);
    fingerprint.ctrlLED(fingerprint.eNormalClose,fingerprint.eLEDBlue,0);
  }else{
    Serial.println("Saving failed");
  }
  Serial.println("-----------------------------");
}

//Fingerprint deletion
void fingerprintDeletion(){
  uint8_t ret;
  /*Compare the captured fingerprint with all the fingerprints in the fingerprint library 
    Return fingerprint ID(1-80) if succeed, return 0 when failed 
   */
  ret = fingerprint.search();
  if(ret){
    /*Set fingerprint LED ring to always ON in green*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDGreen, /*blinkCount = */0);
    fingerprint.delFingerprint(ret);
    Serial.print("deleted fingerprint,ID=");
    Serial.println(ret);
  }else{
    /*Set fingerprint LED ring to always ON in red*/
    fingerprint.ctrlLED(/*LEDMode = */fingerprint.eKeepsOn, /*LEDColor = */fingerprint.eLEDRed, /*blinkCount = */0);
    Serial.println("Matching failed or the fingerprint hasn't been registered");
  }
  delay(1000);
  /*Turn off fingerprint LED ring*/
  fingerprint.ctrlLED(/*LEDMode = */fingerprint.eNormalClose, /*LEDColor = */fingerprint.eLEDBlue, /*blinkCount = */0);
  Serial.println("-----------------------------");
}
