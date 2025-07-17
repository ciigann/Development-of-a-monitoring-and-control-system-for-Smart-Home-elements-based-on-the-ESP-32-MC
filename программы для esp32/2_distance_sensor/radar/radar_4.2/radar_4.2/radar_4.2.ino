/*
MINIMAL EXAMPLE FOR INTERFACING THE LD2450 RADAR SENSOR.
SETUP REQUIRES A LD2450 AND A Esp32S3.

CONNECTIONS:

LD2450 => Esp32S3
5V => VBUS
GND => GND
Red:RX => GPIO17
White:TX => GPIO16
*/

#include <Arduino.h>

#include <LD2450.h>

 
const int ledPin = 2;
String inputString = "";      // a String to hold incoming data
int systemFlag = -1;   
char buffer[200];

// SENSOR INSTANCE
LD2450 ld2450;

void setup()
{
  //SERIAL FOR HOST / DEBUG MESSAGES
  Serial.begin(115200);
  Serial1.begin(256000,SERIAL_8N1,16,17);

  while (!Serial) {
    Serial.println("wait for serial port to connect");
  }

  while (!Serial1) {
    Serial.println("wait for serial1 port to connect");
  }


}

void enConfig() {

  byte enConfig[LD2450_SERIAL_BUFFER] = "";
  enConfig[0]=0xFD;
  enConfig[1]=0xFC;
  enConfig[2]=0xFB;
  enConfig[3]=0xFA;
  enConfig[4]=0x04;
  enConfig[5]=0x00;
  enConfig[6]=0xFF;
  enConfig[7]=0x00;
  enConfig[8]=0x01;
  enConfig[9]=0x00;
  enConfig[10]=0x04;
  enConfig[11]=0x03;
  enConfig[12]=0x02;
  enConfig[13]=0x01;

  Serial1.write(enConfig,sizeof(enConfig));
 
  while (Serial1.available() <= 0) {
    Serial.print('.');  // send a capital A
    delay(1);
  }


  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    Serial.print(inChar);
  }
  systemFlag = 0;

}

void endConfig() {
  byte endConfig[LD2450_SERIAL_BUFFER] = "";
  endConfig[0]=0xFD;
  endConfig[1]=0xFC;
  endConfig[2]=0xFB;
  endConfig[3]=0xFA;
  endConfig[4]=0x02;
  endConfig[5]=0x00;
  endConfig[6]=0xFE;
  endConfig[7]=0x00;
  endConfig[8]=0x04;
  endConfig[9]=0x03;
  endConfig[10]=0x02;
  endConfig[11]=0x01;

  Serial1.write(endConfig,sizeof(endConfig));
 
  while (Serial1.available() <= 0) {
    Serial.print('.');  // send a capital A
    delay(1);
  }


  while (Serial1.available()) {
    // get the new byte:
    char inChar = (char)Serial1.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    Serial.print(inChar);
  }

  systemFlag = 1;

}

void loop()
{
  // print the string when a newline arrives:
  switch (systemFlag){
    case -1:
      Serial.println("Enable config:");
      enConfig();
      delay(3000);
      break;
    case 0:
      Serial.println("End config:");
      endConfig();
      delay(3000);
      break;
    case 1:
        Serial.println("Init LD2450:");
        // BUILD-IN LED TO INDICATED RADAR TARGETS AS AN EXAMPLE
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);

        // SETUP SENSOR
        // HERE THE HARDWARE SERIAL INTERFACE 1 IS USED
        ld2450.begin(Serial1, true);

        if(!ld2450.waitForSensorMessage()){
          Serial.println("SENSOR CONNECTION SEEMS OK");
        }else{
          Serial.println("SENSOR TEST: GOT NO VALID SENSORDATA - PLEASE CHECK CONNECTION!");
        }
        Serial.println("SETUP_FINISHED");
        delay(3000);
        systemFlag = 2;
        break;
    case 2:
      const int sensor_got_valid_targets = ld2450.read();
      if (sensor_got_valid_targets > 0)
      {

        Serial.print(ld2450.getLastTargetMessage());


        // GET THE DETECTED TARGETS
        // TARGET RANGE CAN BE FROM 0 TO ld2450.getSensorSupportedTargetCount(), DEPENDS ON SENSOR HARDWARE. REFER TO LD2450 DATASHEET
        for (int i = 0; i < ld2450.getSensorSupportedTargetCount(); i++)
        {
          const LD2450::RadarTarget result_target = ld2450.getTarget(i);
          //CHECK IF THE TARGET IS MOVING
          // SEE LD2450.h RadarTarget FOR ALL POSSIBLE TARGET DATA SUCH AS X, Y POSITION, DISTANCE,...
          if (result_target.valid && abs(result_target.speed) > 0) // SENSOR SUPPORTS NEGATIVE SPEED IF MOVING TOWARDS SENSOR
          {
            Serial.println("TARGET DETECTED");
            digitalWrite(ledPin, HIGH);
          }
          else
          {
            digitalWrite(ledPin, LOW);
          }
        }
      };
      break;
  }
    /*
    PRINT DEBUG DATA STREAM LIKE THIS: 
    TARGET ID=1 X=-19mm, Y=496mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=496mm, VALID=1
    TARGET ID=2 X=-1078mm, Y=1370mm, SPEED=0cm/s, RESOLUTION=360mm, DISTANCE=1743mm, VALID=1
    TARGET ID=3 X=0mm, Y=0mm, SPEED=0cm/s, RESOLUTION=0mm, DISTANCE=0mm, VALID=0
    */
  // READ FUNCTION MUST BE CALLED IN LOOP TO READ THE INCOMMING DATA STREAM
  // RETURNS -1 or -2 as error flag and 0 to getSensorSupportedTargetCount() if valid targets found

}

  
