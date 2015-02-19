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

#if 0
void uart_write_nibble(uint8_t v)
{
	uint8_t ch;

	if (v < 10)
		ch = '0' + v;
	else
		ch = 'A' + v - 10;

	uart_write_ch(ch);
}

void uart_write_hex(uint8_t v)
{
	uint8_t nibble;

	nibble = (v&0xF0) >> 4;
	uart_write_nibble(nibble);

	nibble = v & 0x0F;
	uart_write_nibble(nibble);
}
#endif

uint8_t uart_read_ch(void)
{
	while (!(USART1_SR & USART_SR_RXNE));
	return USART1_DR;
}

uint8_t uart_read_ch_non_block(void)
{
	if (USART1_SR & USART_SR_RXNE)
		return USART1_DR;
	else
		return 0;
}

void clk_init()
{
	CLK_DIVR = 0x00; // Set the frequency to 16 MHz
	CLK_PCKENR1 = 0xFF; // Enable peripherals
}

void uart_init()
{
	USART1_CR1 = 0; // 8 bits, no parity
	USART1_CR2 = 0;
	USART1_CR3 = 0;

	USART1_BRR2 = 0x04;
	USART1_BRR1 = 0x03; // 38400 baud, order important between BRRs, BRR1 must be last

	USART1_CR2 = USART_CR2_TEN | USART_CR2_REN; // Allow TX & RX
}

void base_configure()
{
	PA_CR1 = 0x00;
	PA_CR2 = 0xFF;
	PA_ODR = 0x00;

	PB_CR1 = 0x00;
	PB_CR2 = 0xFF;
	PB_ODR = 0x00;

	PC_CR1 = 0x00;
	PC_CR2 = 0xFF;
	PC_ODR = 0x00;

	PD_CR1 = 0x00;
	PD_CR2 = 0xCF;
	PD_ODR = 0x00;
}

void output_configure()
{
	PA_DDR = 0xFF;
	PB_DDR = 0xFF;
	PC_DDR = 0xFF;
	PD_DDR = 0x0F;

	base_configure();
}

void input_configure()
{
	PA_DDR = 0x00;
	PB_DDR = 0x00;
	PC_DDR = 0x00;
	PD_DDR = 0x00;

	base_configure();
}

uint8_t process_input(uint8_t port, uint8_t state, uint8_t last_state)
{
	uint8_t pin;

	if (state == last_state)
		return state;

	for (pin = 0; pin < 8; pin++) {
		uint8_t bit = 1 << pin;
		uint8_t cur_pin = (state & bit) ? 1 : 0;
		uint8_t last_pin = (last_state & bit) ? 1 : 0;

		if (port == 'D') {
			if (pin == 5 || pin == 6)
				continue; // UART pins, dont sample
		}

		if (cur_pin != last_pin) {
			uart_write_ch('P');
			uart_write_ch(port);
			uart_write_ch('0' + pin);
			uart_write(" changed from ");
			uart_write_ch('0' + last_pin);
			uart_write(" to ");
			uart_write_ch('0' + cur_pin);
			uart_write("\r\n");
		}
	}

	return state;
}

int main()
{
	uint8_t ch;
	uint8_t r = 1;
	uint8_t p = 0;
	uint8_t c1 = 0;
	uint8_t c2 = 0;
	uint8_t is_output = 1;
	uint8_t last_PA = 0;
	uint8_t last_PB = 0;
	uint8_t last_PC = 0;
	uint8_t last_PD = 0;

	clk_init();
	uart_init();

	output_configure();

	do {
		uint8_t c1v = c1 ? 0xFF : 0x00;
		uint8_t c2v = c2 ? 0xFF : 0x00;
		uint8_t v = 1 << p;

		if (is_output) {
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
				PA_CR1 = c1v;
				PA_CR2 = c2v;
				PA_ODR = v;
			} else if (r == 2) {
				PB_CR1 = c1v;
				PB_CR2 = c2v;
				PB_ODR = v;
			} else if (r == 3) {
				PC_CR1 = c1v;
				PC_CR2 = c2v;
				PC_ODR = v;
			} else if (r == 4) {
				PD_CR1 = c1v;
				PD_CR2 = c2v;
				PD_ODR = v;
			}
		} else {
			// Do input
			PA_CR1 = PB_CR1 = PC_CR1 = PD_CR1 = c1v;
			PA_CR2 = PB_CR2 = PC_CR2 = PD_CR2 = c2v;

			last_PA = process_input('A', PA_IDR, last_PA);
			last_PB = process_input('B', PB_IDR, last_PB);
			last_PC = process_input('C', PC_IDR, last_PC);
			last_PD = process_input('D', PD_IDR, last_PD);
		}

		flush_write();

		if (is_output)
			ch = uart_read_ch();
		else {
			ch = uart_read_ch_non_block();
		}

		if (ch >= 'A' && ch <= 'D')
			r = ch - 'A' + 1;
		else if (ch >= 'a' && ch <= 'd')
			r = ch - 'a' + 1;
		else if (ch >= '0' && ch <= '7')
			p = ch - '0';
		else if (ch == 'Q' || ch == 'q') {
			c1 = 0;
			uart_write("CR1 is now 0\r\n");
		} else if (ch == 'W' || ch == 'w') {
			c1 = 1;
			uart_write("CR1 is now 1\r\n");
		} else if (ch == 'Z' || ch == 'z') {
			c2 = 0;
			uart_write("CR2 is now 0\r\n");
		} else if (ch == 'X' || ch == 'x') {
			c2 = 1;
			uart_write("CR2 is now 1\r\n");
		} else if (ch == 'I' || ch == 'i') {
			is_output = 0;
			input_configure();
			last_PA = last_PB = last_PC = last_PD = 0;
		} else if (ch == 'O' || ch == 'o') {
			is_output = 1;
			output_configure();
		}

	} while(1);
}
