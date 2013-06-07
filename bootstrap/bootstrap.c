#include <mcs51/p89v51rd2.h>

void blinker(void) __interrupt (TF0_VECTOR)
{
	static unsigned short i = 0;
	if (i < 5000) {
		++i;
		return;
	}
	i = 0;
	P0_0 ^= 1;
}

void main()
{
	TMOD = T0_M1;  /* 8-bit auto-reload mode */
	TH0 = -100;  /* call blinker() every 100 cycles */
	ET0 = 1;  /* enable timer 0 interrupt */
	EA = 1;  /* enable interrupts */
	TF0 = 1;  /* trigger the interrupt to init TL0 */
	TR0 = 1;  /* start timer 0 */
	while (1);
}
