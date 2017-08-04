//Nick Reynolds Aug, 18, 2016
//M2LR04 I2C NDEF writer

// using the Wire I2C wrapper functions by Michael Saunby. March 2013.

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
//To use:
//1. Physically connect M2LR Discovery Board to Arduino (UNO board was used in demo, with pins below)
//    -SDA pin to Arduino pin A4
//    -SCL pin to Arduino pin A5 //I2C pins
//    -VCC pin to Arduino pin 5V
//    -GND pin to Arduino pin GND
//2. In setup function below, change "message" array to a new website address if preferred
//    -Right now it writes a webpage displaying a specific elecric imp's collected data
//    -Do not include anything before the domain name (Yes: google.com NO: http://www.google.com)
//    -Right now the header is setup to prepend "https://" to the given url. If you want to change this
//      behavior, go to page 8 of this pdf: (http://www.cardsys.dk/download/NFC_Docs/NFC%20URI%20Record%20Type%20Definition%20Technical%20Specification.pdf)
//      and chose another abbreviation from Table 3. Then replace 0x4 in the "message" array (message[6] in the i2c_eeprom_write_ndef function) with your new value
//    -THIS IS NOT IMPLEMENTED, but if you want Android to automatically open your webpage, you must also include an AAR record after the URI record in the NDEF message. 
//      (see: https://www.nccgroup.trust/us/about-us/newsroom-and-events/blog/2011/november/nfc-intent-filters-in-android-4.0-dont-forget-the-aar/). Info on configuring
//      the byte header needed for this is slim (lots of examples at a higher level).
//3. Use an Android NFC reader app (ex: ST's "NFC-V reader" or "NFC TagInfo") to read your tag and verify the write was correct.
//      If you are using the Discovery Board, you might see "NOT AN NDEF TEXT MESSAGE" across the attached display, this is ok. (Its a URI message, not TEXT)
//
//4. There is a "read byte" function for reading from the M2LR to the Arduino. I think it should work, but it is untested. 
//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------


#include <Wire.h> //I2C library

// Deviceaddress is a 7 bit value. i.e. the read/write bit is missing and the rest
// is shifted right.
// i.e. deviceaddress = 0x53 is sent by Wire as 0xA6 to write or 0xA7 to read.

// Dual interface (I2C+NFC) eeprom
#define M24LR04 0x53 //

void i2c_eeprom_write_byte( int deviceaddress, unsigned int eeaddress, byte data ) {
    int rdata = data;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.write(rdata);
    Wire.endTransmission();
}
  
void i2c_eeprom_write_bytes( int deviceaddress, unsigned int eeaddresspage, byte* data, byte length ) {
    byte c;
    for ( c = 0; c < length; c++){
        Wire.beginTransmission(deviceaddress);
        Wire.write((int)(eeaddresspage >> 8)); // MSB
        Wire.write((int)(eeaddresspage & 0xFF)); // LSB
        Wire.write(data[c]);
        Wire.endTransmission();
        delay(10);
        eeaddresspage++;
    }
}


void i2c_eeprom_write_ndef( int deviceaddress, unsigned int eeaddresspage, byte* data, byte len ) {
  
  unsigned int memInc = 0;

  //NDEF message header
  // E1 | version# | ?(possibly also version#) | ? 
  //changed to align with output of ST's Android NDEF writer. This might not be standardized 
  byte messageHeader[] = {0xE1,0x40,0x40,0x05};
  
  //NDEF record (and first 2 bytes)
  // 03 | nbytes | D1 |01 | payload_length | type | id | payload | FE
  
  byte header[] = {0x03,0x00,0xD1,0x01,0x00,0x55,0x04};
  Serial.println("length is " + len);
  
 
  header[1] = len+4; // Total bytes after D1 marker.
  header[4] = len;
  
  //write message header
  i2c_eeprom_write_bytes( deviceaddress, eeaddresspage, (byte *)messageHeader, sizeof(messageHeader) );
  memInc = memInc + sizeof(messageHeader);

  //write NDEF record header
  i2c_eeprom_write_bytes( deviceaddress, eeaddresspage + memInc, (byte *)header, sizeof(header) );
  memInc = memInc + sizeof(header);

  //write payload
  i2c_eeprom_write_bytes( deviceaddress, eeaddresspage+ memInc, data, len-1 );
  memInc = memInc + len - 1;

  //write stop byte 0xFE and some 0's
  i2c_eeprom_write_bytes( deviceaddress, eeaddresspage + memInc, (byte *)"\xFE\x0\x0\x0\x0\x0\x0\x0\x0",9 );
  
}  

byte i2c_eeprom_read_byte( int deviceaddress, unsigned int eeaddress ) {
  byte rdata = 0xFF;
    Wire.beginTransmission(deviceaddress);
    Wire.write((int)(eeaddress >> 8)); // MSB
    Wire.write((int)(eeaddress & 0xFF)); // LSB
    Wire.endTransmission();
    delay(100);
    Serial.println("before req...");
    delay(100);
    
    Wire.requestFrom(deviceaddress,4);
    delay(100);
    if (Wire.available()){
      rdata = Wire.read();
      //Wire.endTransmission();
      //Serial.println(rdata);
   // }
    }
    return rdata;
  }

void i2c_eeprom_read()
{
  byte newData=0;
  Wire.requestFrom(M24LR04,10);
  Wire.read();
  newData = Wire.read();
  delay(100);
  Serial.println(newData);
/*  while(1)
  {
    
 //   newData = i2c_eeprom_read_byte( M24LR04,0);
    if (Wire.available()){
      newData = Wire.read();
      delay(100);
      Serial.println(newData);
    }
    
  }
*/
}

 void setup() 
  {
    char message[] = "u trash"; // data to write.
    byte returned = 0x00;
    //Wire.begin(); // initialise the connection
    Serial.begin(9600);
    //i2c_eeprom_write_ndef(M24LR04, 0, (byte *)message, sizeof(message));
    Serial.println("Setup done");
    delay(100);
    returned = i2c_eeprom_read_byte( M24LR04, 4);
    Serial.print("The returned value is: ");
    Serial.println(returned);
    //i2c_eeprom_read_byte(M24LR04, 0 );
    //i2c_eeprom_read();
  }
  
  void loop() 
  {
   
  }
