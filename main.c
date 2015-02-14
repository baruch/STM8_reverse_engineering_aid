#include "stm8l.h"
#include <string.h>
#include <stdint.h>

void uart_write_ch(const char ch)
{
	while(!(USART1_SR & USART_SR_TXE));
	USART1_DR = ch;
}

void flush_write()
{
	while(!(USART1_SR & USART_SR_TXE));
}

void uart_write(const char *str)
{
	char i;
	for(i = 0; str[i] != 0; i++) {
		uart_write_ch(str[i]);
	}
}

uint8_t uart_read_ch(void)
{
	while (!(USART1_SR & USART_SR_RXNE));
	return USART1_DR;
}

void clk_init()
{
	CLK_DIVR = 0x00; // Set the frequency to 16 MHz
	CLK_PCKENR1 = 0xFF; // Enable peripherals
}

void uart_init()
{
	USART1_CR1 = 0; // 8 bits, no parity
    USART1_CR3 &= ~(USART_CR3_STOP1 | USART_CR3_STOP2); // 1 stop bit

	USART1_BRR2 = 0x00;
	USART1_BRR1 = 0x0D; // 9600 baud, order important between BRRs, BRR1 must be last

	USART1_CR2 = USART_CR2_TEN | USART_CR2_REN; // Allow TX & RX
//	USART1_CR2 = USART_CR2_TEN; // Allow TX
}

int main()
{
	uint8_t ch;
	unsigned long i = 0;
	uint8_t r = 1;
	uint8_t p = 1;
	uint8_t c1 = 0;
	uint8_t c2 = 0;

	clk_init();
	uart_init();

	PA_DDR = 0xFF;
	PA_CR1 = 0x00;
	PA_CR2 = 0xFF;
	PA_ODR = 0x00;

	PB_DDR = 0xFF;
	PB_CR1 = 0x00;
	PB_CR2 = 0xFF;
	PB_ODR = 0x00;

	PC_DDR = 0xFF;
	PC_CR1 = 0x00;
	PC_CR2 = 0xFF;
	PC_ODR = 0x00;

	PD_DDR = 0x0F;
	PD_CR1 = 0x00;
	PD_CR2 = 0xCF;
	PD_ODR = 0x00;

	do {
		uart_write("Hello World!\r\n");
		uart_write("P");
		uart_write_ch('A' + r - 1);
		uart_write_ch('0' + p);
		uart_write(" CR1 ");
		uart_write_ch('0' + c1);
		uart_write(" CR2 ");
		uart_write_ch('0' + c2);
		uart_write("\r\n");

		PA_ODR = 0;
		PB_ODR = 0;
		PC_ODR = 0;
		PD_ODR = 0;

		if (r == 1) {
			PA_CR1 = c1 ? 0xFF : 0x00;
			PA_CR2 = c2 ? 0xFF : 0x00;
			PA_ODR = 1 << (p-1);
		} else if (r == 2) {
			PB_CR1 = c1 ? 0xFF : 0x00;
			PB_CR2 = c2 ? 0xFF : 0x00;
			PB_ODR = 1 << (p-1);
		} else if (r == 3) {
			PC_CR1 = c1 ? 0xFF : 0x00;
			PC_CR2 = c2 ? 0xFF : 0x00;
			PC_ODR = 1 << (p-1);
		} else if (r == 4) {
			PD_CR1 = c1 ? 0xFF : 0x00;
			PD_CR2 = c2 ? 0xFF : 0x00;
			PD_ODR = 1 << (p-1);
		}
		//for(i = 0; i < 1000000; i++) { } // Sleep

		flush_write();
		ch = uart_read_ch();

		if (ch >= 'A' && ch <= 'D')
			r = ch - 'A' + 1;
		else if (ch >= 'a' && ch <= 'd')
			r = ch - 'a' + 1;
		else if (ch >= '1' && ch <= '8')
			p = ch - '0';
		else if (ch == 'Q' || ch == 'q')
			c1 = 0;
		else if (ch == 'W' || ch == 'w')
			c1 = 1;
		else if (ch == 'Z' || ch == 'z')
			c2 = 0;
		else if (ch == 'X' || ch == 'x')
			c2 = 1;
	} while(1);
}
