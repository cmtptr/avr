#include <avr/io.h>
#include <util/delay.h>

int main(void)
{
	PORTB = 1 << PB4;
	while (1) {
		_delay_ms(500);
		PORTB ^= 1 << PB3 | 1 << PB4;
	}
	return 0;
}
