/*********************************************************************************
 **
 **  LVIFA_Firmware - Provides Functions For Interfacing With The Arduino Uno
 **
 **  Written By:    Sam Kristoff - National Instruments
 **  Written On:    November 2010
 **  Last Updated:  Dec 2011 - Kevin Fort - National Instruments
 **
 **  This File May Be Modified And Re-Distributed Freely. Original File Content
 **  Written By Sam Kristoff And Available At www.ni.com/arduino.
 **
 *********************************************************************************/

#include <Wire.h>
#include <SPI.h>
#include <LiquidCrystal.h>

//Includes for IR Remote
#ifndef IRremoteInt_h
#include "IRremoteInt.h"
#endif
#ifndef IRremote_h
#include "IRremote.h"
#endif



/*********************************************************************************
 ** Optionally Include And Configure Stepper Support
 *********************************************************************************/
#ifdef STEPPER_SUPPORT

  // Stepper Modifications
  #include "AFMotor.h"
  #include "AccelStepper.h"

  // Adafruit shield
  AF_Stepper motor1(200, 1);
  AF_Stepper motor2(200, 2);
  
  // you can change these to DOUBLE or INTERLEAVE or MICROSTEP
  // wrappers for the first motor
  void forwardstep1() {  
    motor1.onestep(FORWARD, SINGLE);
  }
  void backwardstep1() {  
    motor1.onestep(BACKWARD, SINGLE);
  }
  // wrappers for the second motor
  void forwardstep2() {  
    motor2.onestep(FORWARD, SINGLE);
  }
  void backwardstep2() {  
    motor2.onestep(BACKWARD, SINGLE);
  }
  
  AccelStepper steppers[8];  //Create array of 8 stepper objects

#endif

// Variables

unsigned int retVal;
int sevenSegmentPins[8];
int currentMode;
unsigned int freq;
unsigned long duration;
int i2cReadTimeouts = 0;
char spiBytesToSend = 0;
char spiBytesSent = 0;
char spiCSPin = 0;
char spiWordSize = 0;
Servo *servos;
byte customChar[8];
LiquidCrystal lcd(0,0,0,0,0,0,0);
unsigned long IRdata;
IRsend irsend;

// Sets the mode of the Arduino (Reserved For Future Use)
void setMode(int mode)
{
  currentMode = mode;
}

// Checks for new commands from LabVIEW and processes them if any exists.
int checkForCommand(void)
{  
#ifdef STEPPER_SUPPORT
  // Call run function as fast as possible to keep motors turning
  for (int i=0; i<8; i++){
    steppers[i].run();
  }
#endif

  int bufferBytes = Serial.available();

  if(bufferBytes >= COMMANDLENGTH) 
  {
    // New Command Ready, Process It  
    // Build Command From Serial Buffer
    for(int i=0; i<COMMANDLENGTH; i++)
    {
      currentCommand[i] = Serial.read();       
    }     
    processCommand(currentCommand);     
    return 1;
  }
  else
  {
    return 0; 
  }
}

// Processes a given command
void processCommand(unsigned char command[])
{  
  // Determine Command
  if(command[0] == 0xFF && checksum_Test(command) == 0)
  {
    switch(command[1])
    {    
    /*********************************************************************************
    ** LIFA Maintenance Commands
    *********************************************************************************/
    case 0x00:     // Sync Packet
      Serial.print("sync");
      Serial.flush();        
      break;
    case 0x01:    // Flush Serial Buffer  
      Serial.flush();
      break;
      
    /*********************************************************************************
    ** Low Level - Digital I/O Commands
    *********************************************************************************/
    case 0x02:    // Set Pin As Input Or Output      
      pinMode(command[2], command[3]);
      Serial.write('0');
      break;
    case 0x03:    // Write Digital Pin
      digitalWrite(command[2], command[3]);
       Serial.write('0');
      break;
    case 0x04:    // Write Digital Port 0
      writeDigitalPort(command);
       Serial.write('0');
      break;
    case 0x05:    //Tone          
      freq = ( (command[3]<<8) + command[4]);
      duration=(command[8]+  (command[7]<<8)+ (command[6]<<16)+(command[5]<<24));   
   
      if(freq > 0)
      {
        tone(command[2], freq, duration); 
      }
      else
      {
        noTone(command[2]);
      }    
       Serial.write('0');
      break;
    case 0x06:    // Read Digital Pin
      retVal = digitalRead(command[2]);  
      Serial.write(retVal);    
      break;
    case 0x07:    // Digital Read Port
      retVal = 0x0000;
      for(int i=0; i <=13; i++)
      {
        if(digitalRead(i))
        {
          retVal += (1<<i);
        } 
      }
      Serial.write( (retVal & 0xFF));
      Serial.write( (retVal >> 8));    
      break;
      
    /*********************************************************************************
    ** Low Level - Analog Commands
    *********************************************************************************/
    case 0x08:    // Read Analog Pin    
      retVal = analogRead(command[2]);
      Serial.write( (retVal >> 8));
      Serial.write( (retVal & 0xFF));
      break;
    case 0x09:    // Analog Read Port
      analogReadPort();
      break; 
      
    /*********************************************************************************
    ** Low Level - PWM Commands
    *********************************************************************************/      
    case 0x0A:    // PWM Write Pin
      analogWrite(command[2], command[3]);
       Serial.write('0');
      break;
    case 0x0B:    // PWM Write 3 Pins
      analogWrite(command[2], command[5]);
      analogWrite(command[3], command[6]);
      analogWrite(command[4], command[7]);
       Serial.write('0');
      break;
      
    /*********************************************************************************
    ** Sensor Specific Commands
    *********************************************************************************/      
    case 0x0C:    // Configure Seven Segment Display
      sevenSegment_Config(command);
       Serial.write('0');
      break;
    case 0x0D:    // Write To Seven Segment Display
      sevenSegment_Write(command);
       Serial.write('0');
      break;
      
    /*********************************************************************************
    **  I2C
    *********************************************************************************/
    case 0x0E:    // Initialize I2C
      Wire.begin();
       Serial.write('0');
      break;
    case 0x0F:    // Send I2C Data
      Wire.beginTransmission(command[3]);
      for(int i=0; i<command[2]; i++)
      {
        #if defined(ARDUINO) && ARDUINO >= 100
          Wire.write(command[i+4]);
        #else
          Wire.send(command[i+4]);
        #endif
        
      }
      Wire.endTransmission(); 
       Serial.write('0');   
      break;
    case 0x10:    // I2C Read  
      i2cReadTimeouts = 0;
      Wire.requestFrom(command[3], command[2]);
      while(Wire.available() < command[2])
      {
        i2cReadTimeouts++;
        if(i2cReadTimeouts > 100)
        {
          return;
        }
        else
        {
          delay(1); 
        }
      }

      for(int i=0; i<command[2]; i++)
      {
        #if defined(ARDUINO) && ARDUINO >= 100
          Serial.write(Wire.read());
        #else
          Serial.write(Wire.receive());
        #endif
        
      }
      break;
      
    /*********************************************************************************
    ** SPI
    *********************************************************************************/      
    case 0x11:    // SPI Init
      SPI.begin();
       Serial.write('0');
      break;
    case 0x12:    // SPI Set Bit Order (MSB LSB)
      if(command[2] == 0)
      {
        SPI.setBitOrder(LSBFIRST);
      }
      else
      {
        SPI.setBitOrder(MSBFIRST); 
      }  
       Serial.write('0');  
      break;
    case 0x13:    // SPI Set Clock Divider
      spi_setClockDivider(command[2]);
       Serial.write('0');
      break;
    case 0x14:    // SPI Set Data Mode
      switch(command[2])
      {
      case 0:
        SPI.setDataMode(SPI_MODE0);
        break;  
      case 1:
        SPI.setDataMode(SPI_MODE1);
        break; 
      case 2:
        SPI.setDataMode(SPI_MODE2);
        break; 
      case 3:
        SPI.setDataMode(SPI_MODE3);
        break; 
      default:        
        break;
      }    
       Serial.write('0');
      break;
    case 0x15:  // SPI Send / Receive 
      spi_sendReceive(command);
      break;
    case 0x16:  // SPI Close
      SPI.end();
       Serial.write('0');  
      break; 
      
    /*********************************************************************************
    ** Servos
    *********************************************************************************/      
    case 0x17:  // Set Num Servos
      free(servos);
      servos = (Servo*) malloc(command[2]*sizeof(Servo));
      for(int i=0; i<command[2]; i++)
      {
        servos[i] = Servo(); 
      }
      if(servos == 0)
      {
        Serial.write('1');
      }
      else
      {
         Serial.write('0'); 
      }
      break;
    case 0x18:  // Configure Servo
      servos[command[2]].attach(command[3]);
       Serial.write('0');
      break; 
    case 0x19:  // Servo Write
      servos[command[2]].write(command[3]);
       Serial.write('0');
      break;
    case 0x1A:  // Servo Read Angle
      Serial.write(servos[command[2]].read());
      break;
    case 0x1B:  // Servo Write uS Pulse
      servos[command[2]].writeMicroseconds( (command[3] + (command[4]<<8)) );
       Serial.write('0');
      break;
    case 0x1C:  // Servo Read uS Pulse
      retVal = servos[command[2]].readMicroseconds();
      Serial.write ((retVal & 0xFF));
      Serial.write( (retVal >> 8));
      break;
    case 0x1D:  // Servo Detach
      servos[command[2]].detach();
       Serial.write('0');
      break; 
     /*********************************************************************************
         **                                      LCD
         *********************************************************************************/  
    case 0x1E:  // LCD Init
        lcd.init(command[2], command[3], command[4], command[5], command[6], command[7], command[8], command[9], command[10], command[11], command[12], command[13]);
       
        Serial.write('0');
        break;
      case 0x1F:  // LCD Set Size
        lcd.begin(command[2], command[3]);
        Serial.write('0');
        break;
      case 0x20:  // LCD Set Cursor Mode
        if(command[2] == 0)
        {
          lcd.noCursor(); 
        }
        else
        {
          lcd.cursor(); 
        }
        if(command[3] == 0)
        {
          lcd.noBlink(); 
        }
        else
        {
          lcd.blink(); 
        }
        Serial.write('0');
        break; 
      case 0x21:  // LCD Clear
        lcd.clear();
        Serial.write('0');
        break;
      case 0x22:  // LCD Set Cursor Position
        lcd.setCursor(command[2], command[3]);
        Serial.write('0');
        break;
      case 0x23:  // LCD Print
        lcd_print(command);        
        break;
      case 0x24:  // LCD Display Power
        if(command[2] == 0)
        {
          lcd.noDisplay(); 
        }
        else
        {
          lcd.display();
        }
        Serial.write('0');
        break;
      case 0x25:  // LCD Scroll
        if(command[2] == 0)
        {
          lcd.scrollDisplayLeft(); 
        }
        else
        {
          lcd.scrollDisplayRight();
        }
        Serial.write('0');
        break;
      case 0x26: //  LCD Autoscroll
        if(command[2] == 0)
        {
          lcd.noAutoscroll(); 
        }
        else
        {
          lcd.autoscroll(); 
        }
        Serial.write('0');
        break;
      case 0x27: // LCD Print Direction
        if(command[2] == 0)
        {
          lcd.rightToLeft(); 
        }
        else
        {
          lcd.leftToRight(); 
        }
        Serial.write('0');
        break;
      case 0x28: // LCD Create Custom Char
        for(int i=0; i<8; i++)
        {
          customChar[i] = command[i+3];
        }      
        lcd.createChar(command[2], customChar);
        
        Serial.write('0');
        break;
      case 0x29:  // LCD Print Custom Char
        lcd.write(command[2]);
        Serial.write('0');
        break;
            
        
    /*********************************************************************************
    ** Continuous Aquisition
    *********************************************************************************/        
      case 0x2A:  // Continuous Aquisition Mode On
        acqMode=1;
        contAcqPin=command[2];
        contAcqSpeed=(command[3])+(command[4]<<8);  
        acquisitionPeriod=1/contAcqSpeed;
        iterationsFlt =.08/acquisitionPeriod;
        iterations=(int)iterationsFlt;
        if(iterations<1)
        {
          iterations=1;
        }
        delayTime= acquisitionPeriod; 
       if(delayTime<0)
       {
         delayTime=0;
       }   
        break;   
      case 0x2B:  // Continuous Aquisition Mode Off
        acqMode=0;      
        break;  
     case 0x2C:  // Return Firmware Revision
         Serial.write(byte(FIRMWARE_MAJOR));  
         Serial.write(byte(FIRMWARE_MINOR));   
        break;  
     case 0x2D:  // Perform Finite Aquisition
         Serial.write('0');
         finiteAcquisition(command[2],(command[3])+(command[4]<<8),command[5]+(command[6]<<8));
        break;   
    /*********************************************************************************
    ** Stepper
    *********************************************************************************/        
    #ifdef STEPPER_SUPPORT
      case 0x30:  // Configure Stepper
        if (command[2] == 5){    // Support AFMotor Shield
          switch (command[3]){
          case 0:
            steppers[command[3]] = AccelStepper(forwardstep1, backwardstep1);
            break;
          case 1:
            steppers[command[3]] = AccelStepper(forwardstep2, backwardstep2);
            break;
          default:
            break;
          }
        }
        else if(command[2]==6) {                   // All other stepper configurations
          steppers[command[3]] = AccelStepper(1, command[4],command[5],command[6],command[7]);       
        }   
        else{
          steppers[command[3]] = AccelStepper(command[2], command[4],command[5],command[6],command[7]);
        }  
         Serial.write('0');
        break; 
      case 0x31:  // Stepper Write
        AccelStepper_Write(command);
         Serial.write('0');
        break;
      case 0x32:  // Stepper Detach
        steppers[command[2]].disableOutputs();
         Serial.write('0');
        break; 
      case 0x33:  // Stepper steps to go
        retVal = 0;
        for(int i=0; i<8; i++){
          retVal += steppers[i].distanceToGo();
        }
        Serial.write( (retVal & 0xFF) );
        Serial.write( (retVal >> 8) );
  
        break; 
    #endif
    
    /*********************************************************************************
    ** IR Transmit
    *********************************************************************************/
    case 0x34:  // IR Transmit  
      IRdata = ((unsigned long)command [4] << 24) | ((unsigned long)command [5] << 16) | ((unsigned long)command [6] << 8) | ((unsigned long)command [7]);    
      switch(command[2])
      {
        case 0x00:  // NEC
          irsend.sendNEC(IRdata, command[3]);
          break;
        case 0x01:  //Sony
          irsend.sendSony(IRdata, command[3]);
          break;
        case 0x02:  //RC5
          irsend.sendRC5(IRdata, command[3]);
          break;
        case 0x03:  //RC6
          irsend.sendRC6(IRdata, command[3]);
          break;
      }
      Serial.write((IRdata>>16) & 0xFF);
      break;
    /*********************************************************************************
    ** Unknown Packet
    *********************************************************************************/
    default:      // Default Case
      Serial.flush();
      break;     
    }
  }
  else{  
    // Checksum Failed, Flush Serial Buffer
    Serial.flush(); 
  }   
}

/*********************************************************************************
**  Functions
*********************************************************************************/

// Writes Values To Digital Port (DIO 0-13).  Pins Must Be Configured As Outputs Before Being Written To
void writeDigitalPort(unsigned char command[])
{
  digitalWrite(13, (( command[2] >> 5) & 0x01) );
  digitalWrite(12, (( command[2] >> 4) & 0x01) );
  digitalWrite(11, (( command[2] >> 3) & 0x01) );
  digitalWrite(10, (( command[2] >> 2) & 0x01) );
  digitalWrite(9, (( command[2] >> 1) & 0x01) );
  digitalWrite(8, (command[2] & 0x01) );
  digitalWrite(7, (( command[3] >> 7) & 0x01) );
  digitalWrite(6, (( command[3] >> 6) & 0x01) );
  digitalWrite(5, (( command[3] >> 5) & 0x01) );
  digitalWrite(4, (( command[3] >> 4) & 0x01) );
  digitalWrite(3, (( command[3] >> 3) & 0x01) );
  digitalWrite(2, (( command[3] >> 2) & 0x01) );
  digitalWrite(1, (( command[3] >> 1) & 0x01) );
  digitalWrite(0, (command[3] & 0x01) ); 
}

// Reads all 6 analog input ports, builds 8 byte packet, send via RS232.
void analogReadPort()
{
  // Read Each Analog Pin
  int pin0 = analogRead(0);
  int pin1 = analogRead(1);
  int pin2 = analogRead(2);
  int pin3 = analogRead(3);
  int pin4 = analogRead(4);
  int pin5 = analogRead(5);

  //Build 8-Byte Packet From 60 Bits of Data Read
  char output0 = (pin0 & 0xFF);   
  char output1 = ( ((pin1 << 2) & 0xFC) | ( (pin0 >> 8) & 0x03) );
  char output2 = ( ((pin2 << 4) & 0xF0) | ( (pin1 >> 6) & 0x0F) );
  char output3 = ( ((pin3 << 6) & 0xC0) | ( (pin2 >> 4) & 0x3F) );    
  char output4 = ( (pin3 >> 2) & 0xFF);    
  char output5 = (pin4 & 0xFF);
  char output6 = ( ((pin5 << 2) & 0xFC) | ( (pin4 >> 8) & 0x03) );
  char output7 = ( (pin5 >> 6) & 0x0F );

  // Write Bytes To Serial Port
  Serial.print(output0);
  Serial.print(output1);
  Serial.print(output2);
  Serial.print(output3);
  Serial.print(output4);
  Serial.print(output5);
  Serial.print(output6);
  Serial.print(output7);
}

// Configure digital I/O pins to use for seven segment display
void sevenSegment_Config(unsigned char command[])
{
  // Configure pins as outputs and store in sevenSegmentPins array for use in sevenSegment_Write
  for(int i=2; i<10; i++)
  {
    pinMode(command[i], OUTPUT); 
    sevenSegmentPins[(i-1)] = command[i];
  }  
}

//  Write values to sevenSegment display.  Must first use sevenSegment_Configure
void sevenSegment_Write(unsigned char command[])
{
  for(int i=1; i<9; i++)
  {
    digitalWrite(sevenSegmentPins[(i-1)], command[i]);
  }
}

// Set the SPI Clock Divisor
void spi_setClockDivider(unsigned char divider)
{
  switch(divider)
  {
  case 0:
    SPI.setClockDivider(SPI_CLOCK_DIV2);
    break;
  case 1:
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    break;
  case 2:
    SPI.setClockDivider(SPI_CLOCK_DIV8);
    break;
  case 3:
    SPI.setClockDivider(SPI_CLOCK_DIV16);
    break;
  case 4:
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    break;
  case 5:
    SPI.setClockDivider(SPI_CLOCK_DIV64);
    break;
  case 6:
    SPI.setClockDivider(SPI_CLOCK_DIV128);
    break;
  default:
    SPI.setClockDivider(SPI_CLOCK_DIV4);
    break;
  }  
}

void spi_sendReceive(unsigned char command[])
{
  if(command[2] == 1)        //Check to see if this is the first of a series of SPI packets
  {
    spiBytesSent = 0;
    spiCSPin = command[3];    
    spiWordSize = command[4];                    

    // Send First Packet's 8 Data Bytes
    for(int i=0; i<command[5]; i++)
    {
      // If this is the start of a new word toggle CS LOW
      if( (spiBytesSent == 0) || (spiBytesSent % spiWordSize == 0) )
      {              
        digitalWrite(spiCSPin, LOW);                
      }
      // Send SPI Byte
      Serial.print(SPI.transfer(command[i+6]));     
      spiBytesSent++;  

      // If word is complete set CS High
      if(spiBytesSent % spiWordSize == 0)
      {
        digitalWrite(spiCSPin, HIGH);                  
      }
    }
  }
  else
  {
    // SPI Data Packet - Send SPI Bytes
    for(int i=0; i<command[3]; i++)
    {
      // If this is the start of a new word toggle CS LOW
      if( (spiBytesSent == 0) || (spiBytesSent % spiWordSize == 0) )
      {              
        digitalWrite(spiCSPin, LOW);                
      }
      // Send SPI Byte
      Serial.write(SPI.transfer(command[i+4]));     
      spiBytesSent++;  

      // If word is complete set CS High
      if(spiBytesSent % spiWordSize == 0)
      {
        digitalWrite(spiCSPin, HIGH);                  
      }
    }
  } 
}

// Synchronizes with LabVIEW and sends info about the board and firmware (Unimplemented)
void syncLV()
{
  Serial.begin(DEFAULTBAUDRATE); 
  i2cReadTimeouts = 0;
  spiBytesSent = 0; 
  spiBytesToSend = 0;
  Serial.flush();
}

// Compute Packet Checksum
unsigned char checksum_Compute(unsigned char command[])
{
  unsigned char checksum;
  for (int i=0; i<(COMMANDLENGTH-1); i++)
  {
    checksum += command[i]; 
  }
  return checksum;
}

// Compute Packet Checksum And Test Against Included Checksum
int checksum_Test(unsigned char command[])
{
  unsigned char checksum = checksum_Compute(command);
  if(checksum == command[COMMANDLENGTH-1])
  {
    return 0; 
  }
  else
  {
    return 1;
  }
}

// Stepper Functions
#ifdef STEPPER_SUPPORT
  void AccelStepper_Write(unsigned char command[]){
    int steps = 0;
    int step_speed = 0;
    int acceleration = 0;
  
    //Number of steps & speed are a 16 bit values, split for data transfer. Reassemble 2 bytes to an int 16
    steps = (int)(command[5] << 8) + command[6]; 
    step_speed = (int)(command[2] << 8) + command[3];
    acceleration = (int)(command[7] << 8) + command[8];
  
    steppers[command[4]].setMaxSpeed(step_speed);
  
    if (acceleration == 0){
      //Workaround AccelStepper bug that requires negative speed for negative step direction
      if (steps < 0) step_speed = -step_speed;
      steppers[command[4]].setSpeed(step_speed);
      steppers[command[4]].move(steps);
    }
    else {
      steppers[command[4]].setAcceleration(acceleration);
      steppers[command[4]].move(steps);
    }
  }
#endif


void sampleContinously()
{
   
  for(int i=0; i<iterations; i++)
  {
     retVal = analogRead(contAcqPin);
     if(contAcqSpeed>1000) //delay Microseconds is only accurate for values less that 16383
     {
       Serial.write( (retVal >> 2));
       delayMicroseconds(delayTime*1000000); //Delay for neccesary amount of time to achieve desired sample rate    
     }
     else
     {
        Serial.write( (retVal & 0xFF) );
        Serial.write( (retVal >> 8));   
        delay(delayTime*1000);
     }
  }
}
void finiteAcquisition(int analogPin, float acquisitionSpeed, int numberOfSamples)
{
  //want to exit this loop every 8ms
   acquisitionPeriod=1/acquisitionSpeed;
   
  for(int i=0; i<numberOfSamples; i++)
  {
     retVal = analogRead(analogPin);
    
     if(acquisitionSpeed>1000)
     {
       Serial.write( (retVal >> 2));
       delayMicroseconds(acquisitionPeriod*1000000);
     }
     else
     {
       Serial.write( (retVal & 0xFF) );
       Serial.write( (retVal >> 8));
       delay(acquisitionPeriod*1000);
     }
  }
}

void lcd_print(unsigned char command[])
{
  if(command[2] != 0)
  {          
    // Base Specified By User
    int base = 0;
    switch(command[2])
    {
      case 0x01:  // BIN
        base = BIN;
        break;
      case 0x02:  // DEC
        base = DEC;
        break;
      case 0x03:  // OCT
        base = OCT;
        break;
      case 0x04:  // HEX
        base = HEX;
        break;
      default:
        break;
    }
    for(int i=0; i<command[3]; i++)
    {
      lcd.print(command[i+4], base);
    } 
  }
  else
  {
    
    for(int i=0; i<command[3]; i++)
    {
      lcd.print((char)command[i+4]);
    
    } 
  }
  Serial.write('0');
}







