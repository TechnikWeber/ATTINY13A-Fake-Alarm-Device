// attiny13a_fake_alarm.c
// Fake-Alarmanlagen-LED, blitzt alle ~3s kurz auf
// Fuses: lfuse=0x6B, hfuse=0xFF
// LED an PB0, low-active (Kathode an PB0, 3k3 -> VCC)

#define F_CPU 1200000UL

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/wdt.h>
#include <util/delay.h>

#define LED_PIN  PB0

ISR(WDT_vect) {
    // nur aufwachen
}

static void wdt_setup_1s(void) {
    wdt_reset();
    MCUSR &= ~(1 << WDRF);
    WDTCR |= (1 << WDCE) | (1 << WDE);
    WDTCR  = (1 << WDTIE) | (1 << WDP2) | (1 << WDP1);  // ~1.0 s
}

int main(void) {
    DDRB  = 0xFF & ~(1 << PB5);
    PORTB = (1 << LED_PIN);   // LED aus

    ADCSRA &= ~(1 << ADEN);
    ACSR |= (1 << ACD);
    PRR = (1 << PRADC) | (1 << PRTIM0);
    DIDR0 = (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);

    wdt_setup_1s();
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sei();

    uint8_t counter = 0;

    while (1) {
        sleep_mode();

        if (++counter >= 5) {
            counter = 0;
            PORTB &= ~(1 << LED_PIN);   // LED an
            _delay_ms(3);
            PORTB |= (1 << LED_PIN);    // LED aus
        }

        WDTCR |= (1 << WDTIE);
    }
}
