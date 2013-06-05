#include <mcs51/p89v51rd2.h>

void blinker(void) __interrupt (TF0_VECTOR)
{
	P0_0 ^= 1;
}

void main()
{
	P0 = 0xff;
	TMOD = T0_M1;
	TH0 = 0;
	ET0 = 1;
	EA = 0;
	TF0 = 1;
	TR0 = 1;
	while (1);
}
