#include <TimerOne.h>


#include <SPI.h>


//Pin connected to ST_CP of 74HC595
int latchPin = A4;
//Pin connected to SH_CP of 74HC595
int clockPin = 15;
////Pin connected to DS of 74HC595
int dataPin = 16;

int oe = A5;
int gates[] = {
  4, 5, 9, 10, A3};

boolean state = 0;
long lastTime = 0;

int activeDigit = 0;

char disp[5] = {
  ' '};

static const uint16_t
AlphaArray[] PROGMEM = {
  0x0000, // [space]  ASCII 32
  0x00c0, // ! (Yes, lame, but it's the "standard." Better Suggestions welcome.)
  0x0a00, // "
  0xffff, //#
  0x4abf, //$
  0xdab9, //%
  0xd88e, //&
  0x1000, // '
  0x3000, // (
  0x8400, // )
  0xfc30, // *
  0x4830, //+
  0x8000, //  ,
  0x0030, // -
  0x0040, // . (lower DP)
  0x9000, //  /
  0x93cf, // slashed zero
  0x10c0, // 1
  0x806f, // 2
  0x10af, // 3
  0x02f0, // 4
  0x221f, // 5
  0x03bf, //6
  0x5003, // 7
  0x03ff, // 8
  0x02ff, //9
  0x4800, //:
  0x8800, //;
  0x3000, //<
  0x003c, //=
  0x8400, //>
  0x4062, //?
  0x036f, //@
  0xcc0f, //A
  0x48ef, //B
  0x030f, //C
  0x48cf, //D
  0x031f, //E
  0x0313, //F
  0x03af, //G
  0x03f0, //H
  0x480f, //I
  0x01cc, //J
  0x3310, //K
  0x030c, //L
  0x005f, //M
  0x27c0, //N
  0x03cf, //O
  0x0373, //P
  0x23cf, //Q
  0x2373, //R
  0x02bf, //S
  0x4803, //T
  0x03cc, //U
  0x9300, //V
  0xa3c0, //W
  0x02D0, //X
  0x5400, //Y
  0x900f, //Z
  0x480a, // [
  0x2400, // backslash
  0x4805, // ]
  0xa000, // ^
  0x000c, // _
  0x0400, // `

  // Spinners rotate through the middle
#define SPINNER_0 ('`' + 1)
  1 << 0xB,
  1 << 0xC,
  1 << 0x5,
  1 << 0xD,
  1 << 0xE,
  1 << 0xF,
  1 << 0x4,
  1 << 0xA,

  // Chaser goes around the outside of the box
#define CHASER_0 (SPINNER_0 + 8)
  1 << 0x0,
  1 << 0x1,
  1 << 0x6,
  1 << 0x7,
  1 << 0x3,
  1 << 0x2,
  1 << 0x8,
  1 << 0x9,

  // "Thin" fonts
#define THIN_0 (CHASER_0 + 8)
  /*
 * .- -    0 1
   * |\|/|  9ABC6
   *  - -    4 5
   * |/|\|  8FED7
   *  - -.   2 3
   * thin font by "odometer": http://forums.adafruit.com/viewtopic.php?f=41&t=25675#p132457
   */

#define B(x) ((uint16_t) 1 << (x))

  B(1) | B(6) | B(7) | B(3) | B(0xE) | B(0xB), // thin 0
  B(6) | B(7), // thin 1
  B(1) | B(6) | B(0x5) | B(0xE) | B(3), // thin 2
  B(1) | B(6) | B(5) | B(7) | B(3), // thin 3
  B(0xB) | B(5) | B(6) | B(7), // thin 4
  B(1) | B(0xB) | B(5) | B(7) | B(3), // thin 5
  B(1) | B(0xB) | B(5) | B(7) | B(3) | B(0xE), // thin 6
  B(1) | B(6) | B(7), // thin 7
  B(1) | B(0xB) | B(6) | B(5) | B(0xE) | B(7) | B(3), // thin 8
  B(1) | B(0xB) | B(6) | B(5) | B(7) | B(3), // thin 9

  B(9) | B(8) | B(1) | B(6) | B(7) | B(3) | B(0xE) | B(0xB), // thin 10
  B(9) | B(8) | B(6) | B(7), // thin 11
  B(9) | B(8) | B(1) | B(6) | B(0x5) | B(0xE) | B(3), // thin 12
  B(9) | B(8) | B(1) | B(6) | B(5) | B(7) | B(3), // thin 13
  B(9) | B(8) | B(0xB) | B(5) | B(6) | B(7), // thin 14
  B(9) | B(8) | B(1) | B(0xB) | B(5) | B(7) | B(3), // thin 15
  B(9) | B(8) | B(1) | B(0xB) | B(5) | B(7) | B(3) | B(0xE), // thin 16
  B(9) | B(8) | B(1) | B(6) | B(7), // thin 17
  B(9) | B(8) | B(1) | B(0xB) | B(6) | B(5) | B(0xE) | B(7) | B(3), // thin 18
  B(9) | B(8) | B(1) | B(0xB) | B(6) | B(5) | B(7) | B(3), // thin 19

#undef B
};


void setup()
{

  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);
  pinMode(oe, OUTPUT);

  for (int i = 0; i < 5; i++)
  {
    digitalWrite(gates[i], LOW);
    pinMode(gates[i], INPUT);
    //    digitalWrite(gates[i], HIGH); 
  }

  SPI.begin();

  Timer1.initialize(1000);
  Timer1.attachInterrupt( timerISR );

}

void all_digits_off()
{
  for (int i = 0; i < 5; i++)
  {
    pinMode(gates[i], INPUT);
    digitalWrite(gates[i], LOW); 
  }
}

void timerISR() 
{

  all_digits_off();

  digit_write(disp[activeDigit], 0);

  pinMode(gates[activeDigit], OUTPUT);
  digitalWrite(gates[activeDigit], LOW);

  activeDigit++;
  if (activeDigit == 5) activeDigit = 0;
}

static void digit_write(const byte c, const byte dots)
{

  const uint16_t word = pgm_read_word_near(AlphaArray + c - ' ');

  // note that b2/b3 are shuffled relative to the original firmware
  uint8_t b1 = (word >> 10) & 0xff;
  uint8_t b2 = (word >> 8) & 0x03;
  uint8_t b3 = (word & 0xff);

  digitalWrite(oe, HIGH);


  SPI.transfer(0x00);
  SPI.transfer(b1);
  SPI.transfer(b2);
  SPI.transfer(b3);
  

  digitalWrite(latchPin, HIGH);
  digitalWrite(latchPin, LOW);

  digitalWrite(oe, LOW);

}

void loop()
{


   disp[0] = 'M';
   disp[1] = 'A';
   disp[2] = 'X';
   disp[3] = 'X';
   disp[4] = 'X';
  
}

void shiftOut(int myDataPin, int myClockPin, byte myDataOut) {
  // This shifts 8 bits out MSB first, 
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOutÔøΩ
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights. 
  for (i=7; i>=0; i--)  {
    digitalWrite(myClockPin, 0);

    //if the value passed to myDataOut and a bitmask result 
    // true then... so if we are at i=6 and our value is
    // %11010100 it would the code compares it to %01000000 
    // and proceeds to set pinState to 1.
    if ( myDataOut & (1<<i) ) {
      pinState= 1;
    }
    else {	
      pinState= 0;
    }

    //Sets the pin to HIGH or LOW depending on pinState
    digitalWrite(myDataPin, pinState);
    //register shifts bits on upstroke of clock pin  
    digitalWrite(myClockPin, 1);
    //zero the data pin after shift to prevent bleed through
    digitalWrite(myDataPin, 0);
  }

  //stop shifting
  digitalWrite(myClockPin, 0);
}










