#define DEBUG

#include <Arduino.h>
#include "PWMLibs.h"

///////////////////////////////////////////////////////////////////////////////////////////////
// Class ChargePumpPWM
//////////////////////////////////////////////////////////////////////////////////////////////

  ChargePumpPWM::ChargePumpPWM (int OutputA, int OutputB)
    {
      /////////////////////////////////////////////////
      //    Set up the PWM inverting pins and frequency
      /////////////////////////////////////////////////
      OutA = OutputA;
      OutB = OutputB;

      //
      //This bit of code is hardcoded to pins 11 and 3 across the timer.
      //So it  means that passing the pins as parameters doesn't make a lot of sense
      //
      
      TCCR2A = TCCR2A | 0x30; //This sets bits 4&5 to 1 which puts the time 2 
                          //Into inverting mode.
                          //Timer 2 controls pins 11 and 3. 11 Being on A, and 3 on B
      TCCR2B = TCCR2B & 0xF8 | 0x1; // Max frequency, of about 30Khz.
         
      pinMode(OutA,OUTPUT);
      pinMode(OutB,OUTPUT);  
      #ifdef DEBUG
        Serial.print("Pump Charge PWM Signal Establised on Pins");
        Serial.print(OutA);
        Serial.print(" and ");
        Serial.println(OutB);
      #endif
    };

    void ChargePumpPWM::On (void)
    {
       analogWrite(OutA,117);  //Less than (127) 50% duty cycle for non overlapping
       analogWrite(OutB,137);
        #ifdef DEBUG
          Serial.println("ON: Pump Charge PWM Signal turned on");
        #endif
    }

     void  ChargePumpPWM::Off (void)
    {
       analogWrite(OutA,0);  //Sets output low
       analogWrite(OutB,255);  //Sets output low (NB It's inverting)
       #ifdef DEBUG
          Serial.println("OFF: Pump Charge PWM Signal turned OFF");
       #endif
    }
///////////////////////////////////////////////////////////////////////////////////////////////
//END Class ChargePumpPWM
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
//
//  VoltageSensor Class
//  for sensing voltages from Arduino
//  Understands the voltage divider resistors to give a scaled output
//  Note yet implemented is a read under PWM conditions for the battery
/////////////////////////////////////////////////////////////////////////////////////////////

VoltageSensor::VoltageSensor (int Pin, float FullV,int HighR, int LowR)
{
  Readpin = Pin;
  Highside = HighR;
  Lowside = LowR;
  FullScale = FullV;
  PossOver = false;

  //This determines calculates the voltage seen across the Potential Divider
  AcrossLowR =   (float)Lowside / ((float)Lowside+(float)Highside);

  LowRangeConvRatio = 1023 / FullScale;    //This is the ratio that will be needed to convert back to volts from a 0-1023 A-D scale
  FullRangeConvRatio = 1023 / (FullScale / AcrossLowR) ;     //This is the ratio for the full voltage if it were measured.

   #ifdef DEBUG
    Report();
   #endif

   //Check if there is a potential that there will be more than 5 volts on the analogue pin
  if (AcrossLowR > 5)
  {
    PossOver = true;
    #ifdef DEBUG
      Serial.println("**** OverVoltage Possible : Warning Flag Set");
    #endif
  }
}

void VoltageSensor::Report()
{
    Serial.print("\nVoltage Sensor Initialised : Pin ");
    Serial.print(Readpin);
    Serial.print(" Max-Voltage:");
    Serial.print(FullScale);
    Serial.print(" POT-resistors H:");
    Serial.print(Highside);
    Serial.print(" L:");
    Serial.print(Lowside);
    Serial.print(" Lowside Conversion:");
    Serial.print(LowRangeConvRatio);
    Serial.print(" FullS Conversion:");
    Serial.println(FullRangeConvRatio);
    if (PossOver) Serial.println("**** OverVoltage Possible : Warning Flag Set");
}

void VoltageSensor::takeReading (void)
{
  ADReading = analogRead(Readpin);
  LowR = ADReading / LowRangeConvRatio; 
  FullScaledReading = ADReading / FullRangeConvRatio;
  
  #ifdef DEBUG
    Serial.print("\nVoltage Reading Taken: Pin ");
    Serial.print(Readpin);
    Serial.print(" AD Value ");
    Serial.print(ADReading);
    Serial.print(" Full V ");
    Serial.print(FullScaledReading);
    Serial.print("V across divider ");
    Serial.print(LowR);
    Serial.println("V");
  #endif
}

bool VoltageSensor::PossOverVolts (void)
{
  return (PossOver);
}

float VoltageSensor::volts (void)
{
  takeReading();
  return (FullScaledReading);
}

float VoltageSensor::LowReading (void)

{
  takeReading();
  return (LowR); 
}

int VoltageSensor::ADValue (void)
{
  takeReading();
  return (ADReading);
}

///////////////////////////////////////////////////////////////////////////////////////////////
//END Class VoltageSensor
//////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////////
//
//  ChargePWM Class
//  This is for generating the PWM waveform to control the charger
/////////////////////////////////////////////////////////////////////////////////////////////

        ChargePWM::ChargePWM (int InPin)
        {
          PWMPin = InPin;
          pinMode(PWMPin,OUTPUT);
          state = 0;    //Charge is off in initial state
          analogWrite (PWMPin,0);    
        }
        
        void ChargePWM::ImplementWaveForm (int desiredState)
        {
          switch (desiredState)
          {
              case 0: // Turn off
                  analogWrite (PWMPin,0);
                  state=0;
              break;

              case 2: // Do Hard On
                analogWrite (PWMPin,255);
                state=2;
              break;
              
              case 1: // Do smart trickle
                 analogWrite(PWMPin,127);  //50% duty Cycle
                  state=1;      
              break;     
          }
        }

        void ChargePWM::chargeHardOn (void)
        {
          ImplementWaveForm (2);
        }
        
        void ChargePWM::chargeOff (void)
        {
         ImplementWaveForm(0);
        }
        
        void ChargePWM::chargeTrickle (float VG)
        {
          VoltageGap=VG;
          ImplementWaveForm(1);
        }
        
        void ChargePWM::Suspend (void)
        {
          statestore=state;
          chargeOff();
        }
        
        void ChargePWM::UnSuspend (void)
        {
          ImplementWaveForm(statestore);
        }
        
        bool ChargePWM::isTrickle(void)
        {
          if ( state == 1) return true;
          return false;
        }
        
        bool ChargePWM::isOff(void)
        {
          if (state == 0) return true;
          return false;
        }
        
        bool ChargePWM::isHardOn(void)
        {
          if (state == 2) return true;
          return false;
        }
        
///////////////////////////////////////////////////////////////////////////////////////////////
//END Class ChargePWM
//////////////////////////////////////////////////////////////////////////////////////////////

