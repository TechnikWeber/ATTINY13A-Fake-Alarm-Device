/*
 * ATtiny13A low-power blink (optimized)
 *
 * Hardware: LED at PB0, active LOW (LED lights when pin = LOW)
 * Timing:   Flash for ~3 ms every ~5 s
 *
 * Power strategy:
 *   - Default fuses (9.6 MHz / 8 = 1.2 MHz CPU clock, no change needed)
 *   - Power-down sleep between flashes
 *   - Watchdog timer (128 kHz osc) as wake source
 *   - 4 s WDT + 1 s WDT = 5 s total sleep
 *   - ADC, analog comparator, digital input buffers disabled
 *   - Unused pins as input with pull-up (defined state, no leakage)
 *   - BOD disabled in sleep (defensive; no effect if already off via fuse)
 *   - Sleep current: ~4 uA typical
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

// WDT interrupt: just wake up, don't reset
ISR(WDT_vect) {
    // no-op, just wake from sleep
}

// Configure WDT to generate interrupt (not reset) after `prescaler`
static void wdt_setup(uint8_t prescaler) {
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = (1 << WDTIE) | (prescaler & 0x27);
}

// Prescaler bit patterns for WDTCR
#define WDT_1S   ((1 << WDP2) | (1 << WDP1))              // ~1.0 s
#define WDT_4S   ((1 << WDP3))                            // ~4.0 s

static void sleep_wdt(uint8_t prescaler) {
    wdt_setup(prescaler);
    sei();
    sleep_bod_disable();   // disable BOD for sleep (no-op if BOD fuse is off)
    sleep_cpu();
    cli();
    // Disable WDT until next setup
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0x00;
}

int main(void) {
    // --- Pin setup ---
    // PB0 = output HIGH (LED off, active-LOW drive)
    // PB1..PB5 = input with pull-up (defined state, no floating inputs).
    // Note: PB5 is RESET by default; pull-up is internal via the reset circuit,
    // writing PORTB5=1 here is harmless and keeps the state defined.
    DDRB  = (1 << PB0);
    PORTB = (1 << PB0) | (1 << PB1) | (1 << PB2) | (1 << PB3) | (1 << PB4) | (1 << PB5);

    // --- Power savings ---
    ADCSRA &= ~(1 << ADEN);                 // ADC off
    ACSR  |= (1 << ACD);                    // Analog comparator off
    DIDR0 = (1 << ADC0D) | (1 << ADC1D) |   // Disable digital input buffers on
            (1 << ADC2D) | (1 << ADC3D);    //   ADC pins (tiny leak saver)
    PRR = (1 << PRTIM0) | (1 << PRADC);     // Shut down unused peripherals

    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    for (;;) {
        // LED on (active LOW) for ~3 ms
        PORTB &= ~(1 << PB0);
        _delay_ms(3);
        PORTB |=  (1 << PB0);

        // Sleep ~5 s: 4 s + 1 s via watchdog
        sleep_wdt(WDT_4S);
        sleep_wdt(WDT_1S);
    }
}
