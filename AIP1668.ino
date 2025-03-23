/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where can do changes and look to the results     */
/****************************************************/

//set your clock speed
#define F_CPU 16000000UL
//these are the include files. They are outside the project folder
// #include <avr/io.h>
// #include <util/delay.h>
// #include <avr/interrupt.h>

// Standard Input/Output functions 1284
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/atomic.h>
#include <stdbool.h>
#include "stdint.h"
#include <Arduino.h>

#define AIP1668_in 7   // If 0 write LCD, if 1 read of LCD
#define AIP1668_clk 8  // if 0 is a command, if 1 is a data0
#define AIP1668_stb 9  // Must be pulsed to LCD fetch data of bus

#define AdjustPins    PIND // before is C, but I'm use port C to VFC Controle signals

unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);

/*Global Variables Declarations*/
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char minute = 0;
unsigned char secs=0;
unsigned char seconds=0;
unsigned char milisec = 0;

unsigned char memory_secs=0;
unsigned char memory_minutes=0;

unsigned char number;
unsigned char secsU =0x00;
unsigned char numberA1 =0x00;
unsigned char secsD =0x00;
unsigned char numberB1 =0x00;
unsigned char minutU =0x00;
unsigned char numberC1 =0x00;
unsigned char minutD =0x00;
unsigned char numberD1 =0x00;
unsigned char hourU =0x00;
unsigned char numberE1 =0x00;
unsigned char hourD =0x00;
unsigned char numberF1 =0x00;

unsigned char digit=0;
unsigned char grid=0;
unsigned char gridSegments = 0b00000011; // Here I define the number of GRIDs and Segments I'm using

boolean flag=true;
boolean flagSecs=false; // Used in case of using buttons to setting clock, set buttons to do it!

unsigned int segOR[14];  // 7*2 result as 14 positions of map memory to fill table(0b1100xxxx) where x is to address of one grid in normal use!!!
unsigned int wd0 =0x00;
unsigned int wd1 =0x00;
unsigned char segmWheels = 0x00;

//Arrays of bits to Digits They respect standard order of 7 segments display HGFEDCBA
uint8_t numbers[10]={0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x6F}; 

void setup() {
    // put your setup code here, to run once:

    // initialize digital pin LED_BUILTIN as an output.
      pinMode(LED_BUILTIN, OUTPUT);
      Serial.begin(115200);
      seconds = 0x00;
      minutes =0x00;
      hours = 0x00;

      /*CS12  CS11 CS10 DESCRIPTION
      0        0     0  Timer/Counter1 Disabled 
      0        0     1  No Prescaling
      0        1     0  Clock / 8
      0        1     1  Clock / 64
      1        0     0  Clock / 256
      1        0     1  Clock / 1024
      1        1     0  External clock source on T1 pin, Clock on Falling edge
      1        1     1  External clock source on T1 pin, Clock on rising edge
    */
      // initialize timer1 
      cli();           // disable all interrupts
      //initialize timer1 
      //noInterrupts();    // disable all interrupts, same as CLI();
      TCCR1A = 0;
      TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
      
      TCNT1  = 0;
      
      // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
      OCR1A = 62498;            // compare match register 16MHz/256/2Hz
      //OCR1A = 1200; // only to use in test, increment seconds more fast!
      TCCR1B |= (1 << WGM12);   // CTC mode
      TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
      TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt

    // Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
    // a little the value 62499 upper or lower if the clock have a delay or advance on hours.
      
    //  a=0x33;
    //  b=0x01;

    CLKPR=(0x80);
    //Set PORT
    DDRD = 0xFF;  // IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 is port B
    PORTD=0x00;
    DDRB =0xFF;
    PORTB =0x00;

    AIP1668_init();

    clear_AIP1668();

    //only here I active the enable of interrupts to allow run the test of AIP1668
    //interrupts();             // enable all interrupts, is same as sei();
    sei();
}
void AIP1668_init(void){
  delayMicroseconds(200); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!

  // Configure AIP1668 display (grids)
  cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is driver until 7 grids
  delayMicroseconds(1);
  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // set DIMM/PWM to value
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(1);
}
void cmd_without_stb(unsigned char a){
  // send without stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
         for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
           digitalWrite(AIP1668_clk, LOW);
                 if (data & mask){ // if bitwise AND resolves to true
                    digitalWrite(AIP1668_in, HIGH);
                 }
                 else{ //if bitwise and resolves to false
                   digitalWrite(AIP1668_in, LOW);
                 }
          delayMicroseconds(5);
          digitalWrite(AIP1668_clk, HIGH);
          delayMicroseconds(5);
         }
   //digitalWrite(AIP1668_clk, LOW);
}
void cmd_with_stb(unsigned char a){
  // send with stb
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(AIP1668_stb, LOW);
  delayMicroseconds(1);
         for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
           digitalWrite(AIP1668_clk, LOW);
           delayMicroseconds(1);
                 if (data & mask){ // if bitwise AND resolves to true
                    digitalWrite(AIP1668_in, HIGH);
                 }
                 else{ //if bitwise and resolves to false
                   digitalWrite(AIP1668_in, LOW);
                 }
          digitalWrite(AIP1668_clk, HIGH);
          delayMicroseconds(1);
         }
   digitalWrite(AIP1668_stb, HIGH);
   delayMicroseconds(1);
}
void test_AllON(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000000); // Grids of display... 
                            // by this reason I need send 7 blocks of 9 segments, but the total result equal in memory fields!
                            //Grid position;  76543210
                            //Grid position:        98 
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            //First two       .:
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            //First two       .:
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 
                            
                            cmd_without_stb(0b11111111); // 
                            cmd_without_stb(0b00000011); // 

                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void testGrids(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
  unsigned char word1 =0x00;
  unsigned char word0 =0x00;
  for(uint8_t g = 0x00; g < 16; g= g +2){
              for (unsigned int i=256; i>0; i=(i>>1)){
                //Serial.println(i, HEX);
                digitalWrite(AIP1668_stb, LOW);
                delayMicroseconds(1);
                cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids
                cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
                           //                          
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000000 | g); // 
                            //               (..gfedcba)  //
                             word1 = (i & 0xff00UL) >>  8;
                             word0 = (i & 0x00ffUL) >>  0;
                             Serial.print("WD0= "); Serial.print(word0, HEX);
                             Serial.print(", WD1= ");Serial.println(word1, HEX);
                            cmd_without_stb(word0); // Group Low of bits of Digit
                            cmd_without_stb(word1); // Group High of bits of Digit
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(400);
                            clear_AIP1668();
             }
  }
}
void clear_AIP1668(void){
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 14 * 2 bytes
  */
      for (int n=0; n < 14; n++){  // this on the AIP1668 of 6 grids
        //cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is fixed to 5 grids
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(AIP1668_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 7 displays of 7segments, 
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4 set the bright//On SM1628 let the segments "a" of some dsp ON, check!!!
            digitalWrite(AIP1668_stb, HIGH);
            delayMicroseconds(1);
     }
}
void number0(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b00111111); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number1(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b00000110); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number2(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01011011); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number3(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01001111); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number4(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01100110); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number5(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01101101); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number6(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01111101); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number7(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b00000111); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number8(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01111111); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void number9(void){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000100); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b01100111); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(200);
}
void showDigits(uint8_t addr, uint8_t digit){
  //where: 
  //Command 1: Display Mode Setting Command //With Strobe impulse
  //Command 2: Data Setting Command         //With Strobe impulse
  //Command 3: Address Setting Command      //Without Strobe impulse
    digitalWrite(AIP1668_stb, LOW);
    delayMicroseconds(1);
    cmd_with_stb(gridSegments); // cmd 1 // AIP1668 is a drive of 7 grids.
    cmd_with_stb(0b10000000);   // cmd 2 //Normal operation; Set pulse as 1/16
             //
                          digitalWrite(AIP1668_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb(0b11000000 | addr); // Grids of display if filled by two bytes address, increment is active!
                            // by this reason I need send 7 blocks of 9 segments if you want fill all panel!
                            //Seg. position;  76543210 //They respect sequence standard of a display 7 seg: hgfedcba
                            //Seg. position:        98 
                            cmd_without_stb(0b00000000 | digit); // 
                            cmd_without_stb(0b00000000); // 
                            
                            digitalWrite(AIP1668_stb, HIGH);
                            cmd_with_stb((0b10001000) | 7); //cmd 4
                            delay(5);
}
void readButtons(){
  unsigned int inPin = 7;     // pushbutton connected to digital pin 7
  unsigned int val = 0;       // variable to store the read value
  unsigned int dataIn=0;

  byte array[8] = {0,0,0,0,0,0,0,0};
  byte together = 0;

  unsigned char receive = 7; //define our transmit pin
  unsigned char data = 0; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask

  array[0] = 1;

  unsigned char btn1 = 0x41;

      digitalWrite(AIP1668_stb, LOW);
      delayMicroseconds(2);
      cmd_without_stb(0b01000010); // cmd 2 //10=Read Keys; 00=Wr DSP;
      delayMicroseconds(2);
      // cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
      // send without stb
  
  pinMode(7, INPUT_PULLUP);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
  delayMicroseconds(2);
  //PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
  //their current value (0 or 1), be careful setting an input pin though as you may turn 
  //on or off the pull up resistor  
  //This don't send the strobe signal, to be used in burst data send
         for (int z = 0; z < 4; z++){
             //for (mask=00000001; mask > 0; mask <<= 1) { //iterate through bit mask
                   for (int h =8; h > 0; h--) {
                      digitalWrite(AIP1668_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                      delayMicroseconds(2);
                     val = digitalRead(inPin);
                      //digitalWrite(ledPin, val);    // sets the LED to the button's value
                           if (val & mask){ // if bitwise AND resolves to true
                             //Serial.print(val);
                            //data =data | (1 << mask);
                            array[h] = 1;
                           }
                           else{ //if bitwise and resolves to false
                            //Serial.print(val);
                           // data = data | (1 << mask);
                           array[h] = 0;
                           }
                    digitalWrite(AIP1668_clk, LOW);
                    delayMicroseconds(2);
                   } 
             
              Serial.print(z);  // All the lines of print is only used to debug, comment it, please!
              Serial.print(" - " );
                        
                                  for (int bits = 7 ; bits > -1; bits--) {
                                      Serial.print(array[bits]);
                                   }
                        
                        if (z==0){
                           if(array[4] == 1){
                             flagSecs = !flagSecs;  // This change the app to hours or seconds
                           }
                        }
                        
                        if (z==1){
                           if(array[7] == 1){
                              digitalWrite(2, !digitalRead(2));
                          }
                        }
                        
                        if (z==1){
                           if(array[4] == 1){
                             digitalWrite(3, !digitalRead(3));
                             //digitalWrite(AIP1668_onGreen, !digitalRead(AIP1668_onGreen));
                            }
                        } 
                          
                        if (z==2){
                           if(array[7] == 1){
                             digitalWrite(4, !digitalRead(4));
                             //digitalWrite(AIP1668_onGreen, !digitalRead(AIP1668_onGreen));
                            }
                        }                                      
                  Serial.println();
          }  // End of "for" of "z"
      Serial.println();  // This line is only used to debug, please comment it!

 digitalWrite(AIP1668_stb, HIGH);
 delayMicroseconds(2);
 cmd_with_stb((0b10001000) | 7); //cmd 4
 delayMicroseconds(2);
 pinMode(7, OUTPUT);  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
 delay(1); 
}
void count100(void){
  uint8_t dozens = 0x00;
  uint8_t units = 0x00;
  for(uint8_t i = 0x00; i < 100; i++){
    units = i % 10;
    dozens = i / 10;
    Serial.print("Units:  ");Serial.println(units, DEC);
    Serial.print("Dozens: ");Serial.println(dozens, DEC);
    showDigits(0x06, numbers[units]);
    showDigits(0x04, numbers[dozens]);
    delay(500);
  }
}
void randomGet(void){
  uint8_t rnd = 0x00;
  for (uint8_t i = 0x00; i < 100; i++){
  rnd = random(0, 5);
  showDigits(0x02, numbers[rnd]);
  showDigits(0x08, numbers[rnd]);
  delay(100);
  }
}
void showNumbers(void){
    number0();
    delay(250);
    number1();
    delay(250);
    number2();
    delay(250);
    number3();
    delay(250);
    number4();
    delay(250);
    number5();
    delay(250);
    number6();
    delay(250);
    number7();
    delay(250);
    number8();
    delay(250);
    number9();
    delay(250);
}
void loop() {
  testGrids();
  for(uint8_t i =0x00; i < 3; i++){
    clear_AIP1668();
    test_AllON();
    delay(5000);
    clear_AIP1668();
    delay(500);
   }  
   showNumbers();
   clear_AIP1668(); 
   for (uint8_t s = 0x04; s < 0x08; s=s+2){
          for (uint8_t i = 0x00; i < 10; i++){
                showDigits(s, numbers[i]);
                delay(500);
          }
     } 
  clear_AIP1668();
      while(1){
        randomGet();
        clear_AIP1668();
        delay(500);
        count100();
        readButtons();  //This function need be well located, I let it here only to do test!!!
        delay(50);
      }              
}
ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
// https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
      secs++;
      flag = !flag;   
} 
