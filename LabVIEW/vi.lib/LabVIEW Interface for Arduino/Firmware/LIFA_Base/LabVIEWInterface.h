/*********************************************************************************
 **
 **
 **  LVFA_Firmware - Provides Functions For Interfacing With The Arduino Uno
 **
 **  Written By:    Sam Kristoff - National Instruments
 **  Written On:    November 2010
 **  Last Updated:  Dec 2011 - Kevin Fort - National Instruments
 **
 **  This File May Be Modified And Re-Distributed Freely. Original File Content
 **  Written By Sam Kristoff And Available At www.ni.com/arduino.
 **
 *********************************************************************************/


/*********************************************************************************
**  Define Constants
**
**  Define directives providing meaningful names for constant values.
*********************************************************************************/

#define FIRMWARE_MAJOR 02        
#define FIRMWARE_MINOR 00  
#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
#define DEFAULTBAUDRATE 9600    // Defines The Default Serial Baud Rate (This must match the baud rate specifid in LabVIEW)
#else
#define DEFAULTBAUDRATE 115200
#endif
#define MODE_DEFAULT 0            // Defines Arduino Modes (Currently Not Used)
#define COMMANDLENGTH 15          // Defines The Number Of Bytes In A Single LabVIEW Command (This must match the packet size specifid in LabVIEW)
#define STEPPER_SUPPORT 1         // Defines Whether The Stepper Library Is Included - Comment This Line To Exclude Stepper Support


// Declare Variables
unsigned char currentCommand[COMMANDLENGTH];    // The Current Command For The Arduino To Process
//Globals for continuous aquisition
unsigned char acqMode;
unsigned char contAcqPin;
float contAcqSpeed;
float acquisitionPeriod;
float iterationsFlt;
int iterations;
float delayTime;


/*********************************************************************************
**  syncLV
**
**  Synchronizes with LabVIEW and sends info about the board and firmware (Unimplemented)
**
**  Input:  None
**  Output: None
*********************************************************************************/
void syncLV();

/*********************************************************************************
**  setMode
**
**  Sets the mode of the Arduino (Reserved For Future Use)
**
**  Input:  Int - Mode
**  Output: None
*********************************************************************************/
void setMode(int mode);

/*********************************************************************************
**  checkForCommand
**
**  Checks for new commands from LabVIEW and processes them if any exists.
**
**  Input:  None
**  Output: 1 - Command received and processed
**          0 - No new command
*********************************************************************************/
int checkForCommand(void);

/*********************************************************************************
**  processCommand
**
**  Processes a given command
**
**  Input:  command of COMMANDLENGTH bytes
**  Output: 1 - Command received and processed
**          0 - No new command
*********************************************************************************/
void processCommand(unsigned char command[]);

/*********************************************************************************
**  writeDigitalPort
**
**  Write values to DIO pins 0 - 13.  Pins must first be configured as outputs.
**
**  Input:  Command containing digital port data
**  Output: None
*********************************************************************************/
void writeDigitalPort(unsigned char command[]);

/*********************************************************************************
**  analogReadPort
**
**  Reads all 6 analog input ports, builds 8 byte packet, send via RS232.
**
**  Input:  None
**  Output: None
*********************************************************************************/
void analogReadPort();

/*********************************************************************************
**  sevenSegment_Config
**
**  Configure digital I/O pins to use for seven segment display.  Pins are stored in sevenSegmentPins array.
**
**  Input:  Pins to use for seven segment LED [A, B, C, D, E, F, G, DP]
**  Output: None
*********************************************************************************/
void sevenSegment_Config(unsigned char command[]);

/*********************************************************************************
**  sevenSegment_Write
**
**  Write values to sevenSegment display.  Must first use sevenSegment_Configure
**
**  Input:  Eight values to write to seven segment display
**  Output: None
*********************************************************************************/
void sevenSegment_Write(unsigned char command[]);

/*********************************************************************************
**  spi_setClockDivider
**
**  Set the SPI Clock Divisor
**
**  Input:  SPI Clock Divider 2, 4, 8, 16, 32, 64, 128
**  Output: None
*********************************************************************************/
void spi_setClockDivider(unsigned char divider);

/*********************************************************************************
**  spi_sendReceive
**
**  Sens / Receive SPI Data
**
**  Input:  Command Packet
**  Output: None (This command sends one serail byte back to LV for each data byte.
*********************************************************************************/
void spi_sendReceive(unsigned char command[]);

/*********************************************************************************
**  checksum_Compute
**
**  Compute Packet Checksum
**
**  Input:  Command Packet
**  Output: Char Checksum Value
*********************************************************************************/
unsigned char checksum_Compute(unsigned char command[]);

/*********************************************************************************
**  checksum_Test
**
**  Compute Packet Checksum And Test Against Included Checksum
**
**  Input:  Command Packet
**  Output: 0 If Checksums Are Equal, Else 1
*********************************************************************************/
int checksum_Test(unsigned char command[]);

/*********************************************************************************
**  AccelStepper_Write
**
**  Parse command packet and write speed, direction, and number of steps to travel
**  
**  Input:  Command Packet
**  Output: None
*********************************************************************************/
void AccelStepper_Write(unsigned char command[]);
/*********************************************************************************
**  SampleContinuosly
**
**  Returns several analog input points at once.
**
**  Input:  void
**  Output: void
*********************************************************************************/
void sampleContinously(void);

/*********************************************************************************
**  finiteAcquisition
**
**  Returns the number of samples specified at the rate specified.
**
**  Input:  pin to sampe on, speed to sample at, number of samples
**  Output: void
*********************************************************************************/
void finiteAcquisition(int analogPin, float acquisitionSpeed, int numberOfSamples );
/*********************************************************************************
**  lcd_print
**
**  Prints Data to the LCD With The Given Base
**
**  Input:  Command Packet
**  Output: None
*********************************************************************************/
void lcd_print(unsigned char command[]);

