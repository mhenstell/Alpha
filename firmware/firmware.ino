#include <SPI.h>

//Pin connected to ST_CP of 74HC595
int latchPin = A4;
//Pin connected to SH_CP of 74HC595
int clockPin = 15;
////Pin connected to DS of 74HC595
int dataPin = 16;

int oe = A5;
int gates[] = {4, 5, 9, 10, A3};

#define NUM_DIGITS 5

static byte display[NUM_DIGITS];
static byte display_dots[NUM_DIGITS];

byte MainBright = 13;

static const uint16_t
AlphaArray[] PROGMEM =
{
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
    0x03f3, //A
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
    0x17c0, //M
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
    0xb400, //X
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

typedef struct
{
    byte on_period;
    byte off_period;
    byte loops;
} pwm_setting_t;

static const pwm_setting_t
pwm_settings[] PROGMEM =
{
    {   0,   0, 32 }, // 0
    {   0, 240, 32 }, // 1
    {   0,  60, 32 }, // 2
    {   0,  15, 32 }, // 3
    {   0,   3, 32 }, // 4
    {  17, 161, 16 }, // 5
    {  23, 155, 16 }, // 6
    {  33, 145, 16 }, // 7
    {  47, 131, 16 }, // 8
    {  65, 113, 16 }, // 9
    {  87,  91, 16 }, // 10
    { 113,  65, 16 }, // 11
    { 143,  35, 16 }, // 12
    { 177,   1, 16 }, // 13
};

void all_digits_off()
{ 

    cli();
    for (int i = 0; i < 5; i++)
    {
        pinMode(gates[i], INPUT);
        digitalWrite(gates[i], LOW);
    }
    sei();
}

static void digit_write(const byte c, const byte dots)
{

    const uint16_t word = pgm_read_word_near(AlphaArray + c - ' ');

    all_digits_off();

    byte b1 = ((word >> 0) & 0x3F);
    byte b2 = word >> 14;
    byte b3 = word >> 6;

    cli();
    SPI.transfer(0x00);
    SPI.transfer(b1);
    SPI.transfer(b2);
    SPI.transfer(b3);


    digitalWrite(latchPin, HIGH);
    digitalWrite(latchPin, LOW);
    sei();

}

static inline void digit_select(const byte j)
{
    pinMode(gates[j], OUTPUT);
    digitalWrite(gates[j], LOW);
}

/** Turn off the currently selected LED digit driver */
static inline void digit_off(void)
{
    digitalWrite(oe, HIGH);
}

/** Turn on the currently selected LED digit driver */
static inline void digit_on(void)
{
    digitalWrite(oe, LOW);
}

/** Send a character c to LED digit.
 * Blocks until the digit is written and PWM'ed to create the effect.
 */
static inline void digit_write_pwm(const byte digit, const byte c, const byte dots, const byte on_period, const byte off_period)
{
    digit_write(c, dots);
    digit_select(digit);

    // For low brightness levels, we use a very low duty cycle PWM.
    // That's normally fine, but the Arduino interrupts in the background
    // (for example, the one that keeps our millisecond timer accurate!)
    // create tiny timing varations, and so these short "on" bursts
    // can have *very* poor consistency, leading to a jittery, flickery
    // display.  To get around this, we temporarily turn off interrupts,
    // for just long enough to turn on the LEDs for a few clock cycles
    // (under 1 us).   Once interrupts are back on, pending interrupt
    // requests will be serviced, so we should not lose any time in the
    // process.  *However* take great care if you extend this
    // "interrupt free" section of the code to any longer duration.

    if (on_period != 0)
    {
        digit_on();
        delayMicroseconds(on_period);
        digit_off();
    }
    else
    {
        // Lower power mode
        const byte SREGtemp = SREG;
        cli(); // Disable interrupts
        digit_on();

        asm volatile(
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            "nop\n\t"
            ::
        );

        digit_off();
        SREG = SREGtemp; // reenable interrupts.
    }

    if (off_period != 0)
        delayMicroseconds(off_period);
}

static inline void display_char(const byte pos, const byte c)
{
    display[pos] = c;
}

static void display_string(const char WordIn[])
{
    for (byte i = 0 ; i < NUM_DIGITS ; i++)
        display_char(i, WordIn[i]);
}

static void display_refresh(void)
{

    byte bright = MainBright;


    // If the brightness == 0, blank the LED driver and do nothing else
    if (bright == 0)
    {
        all_digits_off();
        return;
    }

    if (bright > 13) bright = 13;

    const pwm_setting_t *const pwm = &pwm_settings[bright];
    const byte loops = pgm_read_byte_near(&pwm->loops);
    const byte on_period = pgm_read_byte_near(&pwm->on_period);
    const byte off_period = pgm_read_byte_near(&pwm->off_period);

    for (byte i = 0 ; i < loops ; i++)
    {
        for (byte j = 0 ; j < 5 ; j++)
        {
            const byte c = display[j];
            const byte dots = display_dots[j];
            digit_write_pwm(j, c, dots, on_period, off_period);
        }
    }
}

void setup()
{

    pinMode(latchPin, OUTPUT);
    pinMode(clockPin, OUTPUT);
    pinMode(dataPin, OUTPUT);
    pinMode(oe, OUTPUT);

    all_digits_off();

    SPI.begin();
    SPI.setClockDivider(SPI_CLOCK_DIV32);
    // SPI.setDataMode(SPI_MODE0);

    SPI.transfer(0);
    SPI.transfer(0);
    SPI.transfer(0);
    SPI.transfer(0);
display_string("     ");
}

void loop()
{
    if (millis() > 1000) display_string("HELLO");
    if (millis() > 5000) display_string("WORLD");

    display_refresh();
}









