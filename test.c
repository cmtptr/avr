#include <avr/interrupt.h>
#include <avr/io.h>

#define F_TIMER0 40000  /* 25-us timer */

static unsigned char speed = 12;
ISR(TIMER0_COMPA_vect)
{
	static unsigned char half_ms, i, j = 160;
	if (++half_ms >= 5) {  /* 0.25 ms */
		half_ms = 0;
		if (++i == j) {
			i = 0;
			PINB = 1 << PB4;
			j = j == speed ? 160 : speed;
		}
	}
}

int main(void)
{
	/* IO port setup */
	DDRB = 1 << DDB4 | 1 << DDB1;  /* configure PB4 and PB1 for output */
	PORTB &= ~(1 << PB4);

	sei();  /* enable interrupts */

	/* PWM timer setup */
	TCCR0A = 1 << WGM01;  /* CTC mode */
	OCR0A = F_CPU / F_TIMER0 - 1;
	TIMSK = 1 << OCIE0A;  /* timer 0 interrupt enabled */
	TCCR0B = 1 << CS00;  /* start timer 0 (no prescaling) */

	/* SPI setup */
	USICR = 1 << USIWM0 | 1 << USICS1;  /* SPI three-wire mode, slave */

	while (1) {
		USIDR = speed;
		USISR = 1 << USIOIF;  /* weirdly enough, this clears the flag */
		while (!(USISR & 1 << USIOIF));
		speed = USIDR;
	}
	return 0;
}
