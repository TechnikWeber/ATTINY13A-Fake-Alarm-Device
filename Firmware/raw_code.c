/*
 * ATtiny13A low-power blink
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
// See ATtiny13A datasheet, WDTCR register
static void wdt_setup(uint8_t prescaler) {
    // Clear WDRF in MCUSR first (recommended by datasheet)
    MCUSR &= ~(1 << WDRF);
    // Timed sequence: set WDCE + WDE to allow changes
    WDTCR |= (1 << WDCE) | (1 << WDE);
    // Write new value: WDT in interrupt mode (WDTIE=1, WDE=0), with prescaler
    WDTCR = (1 << WDTIE) | (prescaler & 0x27);
}

// Prescaler bit patterns for WDTCR (WDP3=bit5, WDP2..0=bits2..0)
#define WDT_1S   ((1 << WDP2) | (1 << WDP1))              // ~1.0 s
#define WDT_4S   ((1 << WDP3))                            // ~4.0 s

static void sleep_wdt(uint8_t prescaler) {
    wdt_setup(prescaler);
    sei();
    sleep_cpu();
    cli();
    // Disable WDT interrupt until next setup
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR = 0x00;
}

int main(void) {
    // --- Pin setup ---
    // PB0 = output, HIGH (LED off, since active-LOW)
    DDRB  = (1 << PB0);
    PORTB = (1 << PB0);

    // --- Power savings ---
    // Disable ADC (big current saver)
    ADCSRA &= ~(1 << ADEN);
    // Disable analog comparator
    ACSR  |= (1 << ACD);
    // Disable digital input buffers on all ADC pins
    DIDR0 = (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);
    // Shut down timers/ADC via PRR (they're unused here)
    PRR = (1 << PRTIM0) | (1 << PRADC);

    // Deepest sleep mode
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();

    for (;;) {
        // LED on (active LOW) for ~3 ms
        PORTB &= ~(1 << PB0);
        _delay_ms(3);
        PORTB |=  (1 << PB0);

        // Sleep ~5 s total: 4 s + 1 s
        sleep_wdt(WDT_4S);
        sleep_wdt(WDT_1S);
    }
}
