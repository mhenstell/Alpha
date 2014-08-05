#include <SPI.h>
#include "font.h"
#include <OSCBundle.h>
#include <SLIPEncodedUSBSerial.h>

SLIPEncodedUSBSerial SLIPSerial(Serial);

#define NUM_DIGITS 5


int latchPin = A4;
int clockPin = 15;
int dataPin = 16;

int oe = A5;
int gates[] = {4, 5, 9, 10, A3};

static byte display[NUM_DIGITS];
static byte display_dots[NUM_DIGITS];

byte MainBright = 13;

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

static inline void digit_off(void)
{
    digitalWrite(oe, HIGH);
}

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

    if (off_period != 0) delayMicroseconds(off_period);
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

void LEDcontrol(OSCMessage &msg)
{
    if (!msg.isString(0)) return;

    int length = msg.getDataLength(0);
    if (length != NUM_DIGITS) return;

    char str[length];
    msg.getString(0, str, length);
    display_string(str);

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

    SLIPSerial.begin(9600);
    while(!Serial);
}

void loop()
{

    OSCBundle bundleIN;
    int size;

    while (!SLIPSerial.endofPacket())
    {
        if ((size = SLIPSerial.available()) > 0)
        {
            while (size--) bundleIN.fill(SLIPSerial.read());
        }
    }

    if (!bundleIN.hasError()) bundleIN.dispatch("/led", LEDcontrol);

    display_refresh();
}