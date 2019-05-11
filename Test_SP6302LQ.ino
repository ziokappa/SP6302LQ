// This file is ment to explain how the chipset SP6302LQ works. 
// I have found a chipset already connected to a 15 character display on a board named (PCB-088B-DISP-B)
// Is a board that I have found inside an old Cable TV decoder.
//
// THE SPI PROTOCOL: 
// =================
// The chipset communicate with the controller through a SPI channel.
// Arduino implements the SPI protocol. I am using Arduino Uno that uses pin 10, 11, 12, 13
// SPI protocol is a serial master-slave communication technique that allows you to let two component to 
// exchange information. In the communication there's always a Master (typically the microcontroller as 
// Arduino in this case) and a Slave. 
// The way SPI names the pin is quite interesting: 
// Pin 11   is MOSI that means "Master Output Slave Input" hence when the device it is located on is 
//          used as Master it works as Output pin but if the device it is located on is used as Slave 
//          it works as Input. This means that when you connect a MOSI pin on your Master with the MOSI 
//          pin on your slave your are creating a flow of information from MASTER to SLAVE. 
// Pin 12   is MISO (Master Input Slave Output) it works in the opposite direction compared to Pin 11.
// Pin 13   Is the Clock pin. Typically on every serial communication the master device generates the 
//          clock signal to syncronize the communication between the two peers. 
// Pin 10   Slave Select is the pin that the Master use to start and stop the communication towards the 
//          the slave. When the pin is set to LOW the communication can start and when it is HIGH the 
//          the communication stops. 
// 
// The communication with a display is one direction only, from Master (the Ardiuno) to Slave (the display)
// Hence only the MOSI pins are used and not the MISO ones. 
// Be aware that the name of the pins on the chipset SP6302LQ are different but the following is the right mapping:
//          - MOSI is named DIN
//          - Clock is CLKB
//          - Slave Select (SS) is CSB. 
// 
// THE SP6302LQ CHIPSET: 
// =====================
// To write the text on the display you have to write data into the DCRAM (Direct Control RAM). 
// Typically you send the first byte that represents the command for writing a character in the specified position
// followed by a byte representing the character you want to display according to the character table stored in the 
// chipset ROM (you can find all the codes in the documentation). Now if you send another character it is automatically
// written in the next position (you do not have to specify explicitly the next position). 
// So the typical loop should be: 
// - Set the SS pin to LOW
// - Send the command to write in the DC RAM in position 0
// - Send the character to be displaid in position 0
// - Send the character to be displaid in position 1
// - ....
// - ...
// - Send the character to be displaid in position 14 
// - Set the SS pin to HIGH
//
// There are also other commands that you can use to adjust the number of character to be used; the intensity of the light; ...

// inslude the SPI library:
#include <SPI.h>

// set pin 10 as the slave select
const int slaveSelectPin = 10;
const int pausa = 7; 
char listaChar[] =  {' ',  'A',  'B',  'C',  'D',  'E',  'F',  'G',  'H',  'I',  'J',  'K',  'L',  'M',  'N',  'O',  'P',  'Q',  'R',  'S',  'T',  'U',  'V',  'W',  'X',  'Y',  'Z' };
byte codifica[] =   {0x20, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5A};
int sizeListaChar = sizeof(listaChar)/sizeof(char);


void setup() {
  
  Serial.begin(9600);
  
  byte value; 
  byte conf;

  // Set the slaveSelectPin as an output:
  pinMode(slaveSelectPin, OUTPUT);
  
  // Initialize SPI:
  SPI.begin();
  SPI.beginTransaction(SPISettings(14000000, LSBFIRST, SPI_MODE3));
  SPI.endTransaction();
  
  //Settaggio numero di digit
  value = B01100000; 
  digitalVFDWrite(value);
  delay (500);

  // Display in normal mode
  value = B01110000;
  digitalVFDWrite(value);
  delay (500);

  // Azzero la ADRAM
  digitalWrite(slaveSelectPin, LOW);
  conf = B00000000;
  value = B00110000;
  SPI.transfer(value);
  delay(pausa);
  for (int i=0; i<16; i++){ 
    SPI.transfer(conf);
    delay(pausa);
  }  
  digitalWrite(slaveSelectPin, HIGH);

  // Azzero la DCRAM
  digitalWrite(slaveSelectPin, LOW);
  conf = B00100000; // blank
  value = B00010000;
  SPI.transfer(value);
  delay(100);
  for (int i=0; i<16; i++){ 
    SPI.transfer(conf);
    delay(pausa);
  }  
  digitalWrite(slaveSelectPin, HIGH);

 
}

void loop() {
  displayString ( "PAOLO VITTORIO CAPITELLI" );
  delay (1000);
}

// This function is used to send a one byte command
// to the chipset. Infact it starts the transaction
// putting the Select Slave PIN (SS) to LOW, sends 
// the command via the SPI protocol and closes the 
// transaction putting the SS pin back to HIGH. 
void digitalVFDWrite(int value) {
  
  // Starts the transaction putting the 
  // Slave Select PIN (SS) to LOW.
  digitalWrite(slaveSelectPin, LOW);
  delay(pausa);
  
  // Send the command / value via SPI protocol
  SPI.transfer(value);
  delay(pausa);
  
  // Ends the transaction putting the 
  // Slave Select PIN (SS) to HIGH.
  digitalWrite(slaveSelectPin, HIGH);
}


// Takes the input string, truncates it to 15 characters 
// (the size of the display); send all the characters 
// from posizion 0x0 (the rightmost) to position 0XF in 
// reverse order (from the last character to the first)
void displayStringTrunc( String testo){
  
  byte codiceChar;
  byte value;

  if ( testo.length() > 15 ) {
    testo = testo.substring(0,15);
  }

  // Starts the transaction putting the 
  // Slave Select PIN to Low.
  digitalWrite(slaveSelectPin, LOW);
  
  // Command: Start writing in DCRAM in position 0 (the rightmost)
  value = B00010000; 
  SPI.transfer(value);
  delay(pausa);

  for (int i=testo.length()-1; i>=0; i--) {
    codiceChar = decodeChar( testo.charAt(i)); 
    SPI.transfer( codiceChar );
    delay(pausa); 
  }

  // Ends the transaction putting the 
  // Slave Select PIN to High.
  digitalWrite(slaveSelectPin, HIGH);
  
}

// Takes the input string and if it is wider than
// 15 characters it extract in turns portions of
// the string wider 15 characters and display it 
// using the displayStringTrunc() function.
// The first portion is from char 0 to char 14, the
// second portion is from char 1 to char 15 ...
// The effect is that the entire text scrolls on 
// the screen from right to left. 
void displayString( String testo){
  
  byte codiceChar;
  byte value;
  int lunghezza = testo.length();

  if ( lunghezza <= 15 ) {
      displayStringTrunc ( testo );
  } else {
      for (int i=0; i<=(lunghezza-15); i++)
      {
        displayStringTrunc ( testo.substring(i,i+15) );
      }
  }
}

// Given a character returns the bytecode of it 
// according to the char table stored within the 
// SP6302LQ chipset. The char table is stored in 
// the two arrays listaChar[] and codifica[]
// In this example only uppercase characters and 
// blank has been defined but using the SP6302LQ
// datasheet you can define the whole char set. 
byte decodeChar(char carattereIn) 
{
    for (int j=0; j<sizeListaChar; j++)
    {
      if (carattereIn == listaChar[j])
      {
        return codifica[j];
      }
    }
}
