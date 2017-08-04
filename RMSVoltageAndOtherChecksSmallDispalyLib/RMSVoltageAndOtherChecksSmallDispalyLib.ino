
//Nick Reynolds
//June 2, 2017
//reads the AC voltage and returns an RMS value to serial, eventually to a display.
//tests Permiability, resistance, and continuity, and displays


/*---------------Libraries--------------------------------------------------------------------*/
#include "SSD1306Ascii.h"
#include "SSD1306AsciiAvrI2c.h"

/*---------------Pin Settings-----------------------------------------------------------------*/

//for resistance measurement:
int CT_Pin1 = A2;
int CT_Pin2 = 6;
int CT_Pin3 = 5;
int CT_Pin4 = 4;
int CT_Wind_Measure_Pin = A0;
int CT_Short_Measure_Pin = A1;

//for display:

//for buttons:

/*---------------Defines--------------------------------------------------------------------*/

//conversion factors for current measure
#define InitialScalingFactor 10000
#define OpAmpGain 120.0
#define MVToMAFactor .930000 //this was experimentally measured, with the intention to have to have the
                         //most accuracy around the 5 mA range.


//thresholds for other tests
#define V_CT_LOW  1.78 //min for winding resistance tolerance
#define V_CT_HIGH  1.91 //max for winding resistance tolerance

#define V_SHORT_LOW  2.4 //min for winding resistance tolerance
#define V_SHORT_HIGH  2.6 //max for winding resistance tolerance

#define V_CC_THRESH  4.5 //min for Vcc sensing
#define V_CC_LOW_THRESH  .5 //min for Vcc sensing

// 0X3C+SA0 - 0x3C or 0x3D
#define I2C_ADDRESS 0x3D
SSD1306AsciiAvrI2c oled;

/*---------------Variables-------------------------------------------------------------------*/

//for other tests
float CTVoltageMeasurement = 0;
float CTShortMeasurement = 0;

boolean resistancePass = false;
boolean shortPass = false;
boolean oneThreeOpenPass = false;
boolean twoFourOpenPass = false;

//for voltage measure
unsigned long time;
float sum;
float average;
float scaledValue;


int Vmeasure[500];
int k;


/*----------------------------------------------------------------------------------------------------*/
/*--------------------------------------------- Code start -------------------------------------------*/
/*----------------------------------------------------------------------------------------------------*/

void setup()
{
  
  //init other tests
  pinMode(CT_Pin1, OUTPUT);
  pinMode(CT_Pin2, OUTPUT);
  pinMode(CT_Pin3, OUTPUT);
  pinMode(CT_Pin4, OUTPUT);
  pinMode(CT_Wind_Measure_Pin, INPUT);
  pinMode(CT_Short_Measure_Pin, INPUT);
  Serial.begin(9600);
  
  //init Display:
  oled.begin(&Adafruit128x64, I2C_ADDRESS);
  oled.setFont(Adafruit5x7);
  oled.clear();  

  //for current measurement
  Serial.begin(9600);
  k=0;
  time = millis();
}


void loop()
{
  Vmeasure[k] = analogRead(A3);
  k++;

  if(k>499)
  {
    //error!
    Serial.println("error! mem overflow");
    Serial.println(time);
    Serial.println(millis());
    while(1);
  }
  else if(millis() >= time + 500)
  {
    //calculate
    //calculateRMS();
   // displayResistance();
   testShort();
   testOneThreeOpen();
   testTwoFourOpen();
   displayContinuity();
  
   while(1);
    
  }
 delay(1);
}

/*----------------------------------------------------------------------------------------------------*/
/*------------------------------------ RMS functions --------------------------------------------------*/
/*----------------------------------------------------------------------------------------------------*/

void calculateRMS()
{
  sum = 0;
  for(int i=0;i<400;i++)
  {
    sum = sum + Vmeasure[i];
  }

  average = sum/400; //average is dc component

  //Serial.print("average1: ");
  //Serial.println(average);

  //remove dc component and make numbers all positive to measure "area under curve"
  for(int i=0;i<400;i++)
  {
    Vmeasure[i] = Vmeasure[i]-(int)average;
    Vmeasure[i] = abs(Vmeasure[i]);
  }

  sum=0;
  // sum new total area
  for(int i=0;i<400;i++)
  {
    sum = sum + Vmeasure[i];
  }

  //get new average of total area

  average = sum/400;
 // Serial.print("average2: ");
  //Serial.println(average);
  
  scaledValue = average*(5000.0/1024.0)*MVToMAFactor/OpAmpGain;

  Serial.println(scaledValue);
  delay(200);
  
  k=0;
  time = millis();
}

/*----------------------------------------------------------------------------------------------------*/
/*----------------------------------------- Other Test Functions --------------------------------------*/
/*----------------------------------------------------------------------------------------------------*/

void testResistance()
{
  analogWrite(CT_Pin1,255);
  digitalWrite(CT_Pin2,LOW);
  digitalWrite(CT_Pin3,LOW);
  digitalWrite(CT_Pin4,LOW);
  delay(10);
  CTVoltageMeasurement = analogRead(CT_Wind_Measure_Pin);
  CTVoltageMeasurement = 5*(CTVoltageMeasurement/1024.0);

  if( CTVoltageMeasurement >= V_CT_LOW && CTVoltageMeasurement <= V_CT_HIGH)
  {
    //good Pin 1/2 Resistance
    Serial.println("good 1 2 resistance");
    
    resistancePass = true;
    //displayResistance();
  }
  else
  {
    //bad Pin 1/2 Resistance
    Serial.println("bad 1 2 resistance");
    
    Serial.println(CTVoltageMeasurement);
    //displayResistance();
  }
  analogWrite(CT_Pin1,0);
}

void testShort()
{
  analogWrite(CT_Pin1,0);
  digitalWrite(CT_Pin2,LOW);
  digitalWrite(CT_Pin3,HIGH);
  digitalWrite(CT_Pin4,LOW);

  delay(10);
  
  CTShortMeasurement =  analogRead(CT_Short_Measure_Pin);
  CTShortMeasurement = 5*(CTShortMeasurement/1024.0);
  
  if( CTShortMeasurement >= V_SHORT_LOW && CTShortMeasurement <= V_SHORT_HIGH)
  {
    //good trip wire
    Serial.println("good trip wire");
    
    shortPass = true;
  }
  else
  {
    //bad trip wire
    Serial.println("bad trip wire");
    //Serial.print(CTShortMeasurement);
    //Serial.println();
    
  }

  digitalWrite(CT_Pin3,LOW);
}

void testOneThreeOpen()
{
  analogWrite(CT_Pin1,255);
  digitalWrite(CT_Pin2,LOW);
  digitalWrite(CT_Pin3,LOW);
  digitalWrite(CT_Pin4,LOW);

  if( CTShortMeasurement > V_CC_THRESH)
  {
    //bad. pins are shorted
    Serial.println("1 3 shorted");
  }
  else
  {
    //good. pins are not shorted
    Serial.println("1 3 ok");

    oneThreeOpenPass = true;
  }
  
  analogWrite(CT_Pin1,0);
}

void testTwoFourOpen()
{
  analogWrite(CT_Pin1,0);
  digitalWrite(CT_Pin2,LOW);
  digitalWrite(CT_Pin3,LOW);
  digitalWrite(CT_Pin4,HIGH);

  if( CTVoltageMeasurement > V_CC_THRESH)
  {
    //bad. pins are shorted
    Serial.println("2 4 shorted");
  }
  else
  {
    //good. pins are not shorted
    Serial.println("2 4 ok");

    twoFourOpenPass = true;
  }
  
  analogWrite(CT_Pin1,0);
}


/*----------------------------------------------------------------------------------------------------*/
/*----------------------------------------- Display Functions ----------------------------------------*/
/*----------------------------------------------------------------------------------------------------*/


void displayResistance()
{
  oled.clear();
  oled.set2X();
  oled.println("RESISTANCE");
  oled.print(CTVoltageMeasurement);
  oled.print(" Ohm");
}


void displayContinuity()
{
  int numFails = 0;
  
  if(!resistancePass){numFails++;}
  if(!shortPass){numFails++;}
  if(!oneThreeOpenPass){numFails++;}
  if(!twoFourOpenPass){numFails++;}

  if(numFails > 1)
  {
    Serial.println("Mult fails");
    oled.clear();
    oled.set2X();
    oled.println("MULTIPLE");
    oled.println("CONTINUITY");
    oled.println("FAILURES");
  }
  else if(numFails > 0)
  {
    if(!resistancePass) //fails resistance test
    {
      Serial.println("RESISTANCE failure");
      
      oled.clear();
      oled.set2X();
      oled.println("RESISTANCE");
      oled.println("FAILURE");
      oled.println(CTVoltageMeasurement); //temp. REMOVE!!!
      //oled.println("X Ohms"); 
    }
   if(!shortPass) //fails short test
    {
      Serial.println("short failure");
      
      oled.clear();
      oled.set2X();
      oled.println("TRIP WIRE");
      oled.println("FAILURE");
    }
   if(!oneThreeOpenPass) //fails 1-3 test
    {
      Serial.println("1-3 failure");
      
      oled.clear();
      oled.set2X();
      oled.println("PIN 1 3");
      oled.println("SHORT");
      oled.println("FAILURE");
    }
   if(!twoFourOpenPass) //fails 2-4 test
    {
      Serial.println("2-4 failure");
      
      oled.clear();
      oled.set2X();
      oled.println("PIN 2 4");
      oled.println("SHORT");
      oled.println("FAILURE");
    }
  }
  else
  {
    oled.clear();
    oled.set2X();
    oled.println("ALL");
    oled.println("TESTS");
    oled.println("PASS");
  } 
}


