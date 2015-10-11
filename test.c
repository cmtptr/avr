#include <avr/io.h>

int main(void)
{
	/* IO port setup */
	DDRB = 1 << DDB4 | 1 << DDB1;  /* configure PB4 and PB1 for output */

	/* PWM timer setup */
	GTCCR = 1 << PWM1B | 1 << COM1B1;  /* PWM generation on, output ORC1B */
	OCR1B = 12;  /* 1.536-ms pulse width */
	OCR1C = 155;  /* 19.968-ms period */
	TCCR1 = 1 << CS13;  /* start the timer at F_CPU / 128 */

	/* SPI setup */
	USICR = 1 << USIWM0 | 1 << USICS1;  /* SPI three-wire mode, slave */

	char speed = 0;
	while (1) {
		USIDR = speed;
		USISR = 1 << USIOIF;  /* weirdly enough, this clears the flag */
		while (!(USISR & 1 << USIOIF));
		speed = USIDR;
		if (speed > 4)
			speed = 4;
		else if (speed < -4)
			speed = -4;
		OCR1B = 12 - speed;
	}
	return 0;
}
