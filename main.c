#include "stm8s.h"
#include <string.h>
#include <stdint.h>

uint8_t readbuf[255];
uint8_t readbuf_wr_head;
uint8_t readbuf_rd_head;

uint8_t last_pa_idr;
uint8_t last_pb_idr;
uint8_t last_pc_idr;
uint8_t last_pd_idr;

uint8_t scan;

void readbuf_init(void)
{
	readbuf_wr_head = 0;
	readbuf_rd_head = 255;
}

void readbuf_append(uint8_t ch)
{
	if (readbuf_rd_head == 255)
		readbuf_rd_head = readbuf_wr_head;
	readbuf[readbuf_wr_head++] = ch;
	if (readbuf_wr_head == 255)
		readbuf_wr_head = 0;
}

uint8_t readbuf_available(void)
{
	return readbuf_rd_head != 255;
}

uint8_t readbuf_read(void)
{
	uint8_t ch = readbuf[readbuf_rd_head];
	readbuf_rd_head++;
	if (readbuf_rd_head == 255)
		readbuf_rd_head = 0;
	if (readbuf_rd_head == readbuf_wr_head)
		readbuf_rd_head = 255;
	return ch;
}

uint8_t uart_read_ch_non_block(void)
{
	uint8_t sr = USART1_SR;
	if (sr & USART_SR_RXNE)
		return USART1_DR;
	else
		return 0;
}

void uart_write_ch(const char ch)
{
	while (1) {
		uint8_t sr = USART1_SR;
		USART1_SR = 0;
		if (sr & USART_SR_RXNE) {
			readbuf_append(USART1_DR);
		} else if (sr & USART_SR_TXE) {
			USART1_DR = ch;
			break;
		}
	}
}

void uart_write(const char *str)
{
	char i;
	for(i = 0; str[i] != 0; i++) {
		uart_write_ch(str[i]);
	}
}

void uart_write_nibble(uint8_t v)
{
	uint8_t ch;

	if (v < 10)
		ch = '0' + v;
	else
		ch = 'A' + v - 10;

	uart_write_ch(ch);
}

void uart_write_hex_8(uint8_t v)
{
	uint8_t nibble;

	nibble = (v&0xF0) >> 4;
	uart_write_nibble(nibble);

	nibble = v & 0x0F;
	uart_write_nibble(nibble);
}

void clk_init()
{
	CLK_CKDIVR = 0x00; // Set the frequency to 16 MHz
}

void uart_init()
{
	USART1_CR1 = 0; // 8 bits, no parity
	USART1_CR2 = 0;
	USART1_CR3 = 0;

	USART1_BRR2 = 0x01;
	USART1_BRR1 = 0x1A; // 38400 baud, order important between BRRs, BRR1 must be last

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

#define TYPE_ODR 0
#define TYPE_IDR 1
#define TYPE_DDR 2
#define TYPE_CR1 3
#define TYPE_CR2 4



void show_hex(uint16_t val)
{
	uart_write_hex_8( (val>>8) & 0xFF);
	uart_write_hex_8(val & 0xFF);
	uart_write_ch(' ');
}

unsigned char *get_port_type_addr(uint8_t port, uint8_t type)
{
	unsigned char *p = (unsigned char *)0x5000;

	port -= 'A';
	p += port*5; // Select port
	p += type; // Select register


	return p;
}

uint8_t get_pin_info(uint8_t port, uint8_t pin, uint8_t type)
{
	unsigned char *p = get_port_type_addr(port, type);
	return ((*p) & (1<<pin)) ? 1 : 0;
}

void pin_turn_on(uint8_t port, uint8_t pin, uint8_t type)
{
	unsigned char *p = get_port_type_addr(port, type);
	*p |= (1<<pin);
}

void pin_turn_off(uint8_t port, uint8_t pin, uint8_t type)
{
	unsigned char *p = get_port_type_addr(port, type);
	*p &= ~(1<<pin);
}

uint8_t get_pin_input(uint8_t port, uint8_t pin)
{
	return get_pin_info(port, pin, TYPE_IDR);
}

uint8_t get_pin_output(uint8_t port, uint8_t pin)
{
	return get_pin_info(port, pin, TYPE_ODR);
}

uint8_t get_pin_is_output(uint8_t port, uint8_t pin)
{
	return get_pin_info(port, pin, TYPE_DDR);
}

uint8_t get_pin_cr1(uint8_t port, uint8_t pin)
{
	return get_pin_info(port, pin, TYPE_CR1);
}

uint8_t get_pin_cr2(uint8_t port, uint8_t pin)
{
	return get_pin_info(port, pin, TYPE_CR2);
}

uint8_t port;
uint8_t pin;

void display_pin_state(uint8_t port, uint8_t pin)
{
	uart_write_ch('P');
	uart_write_ch(port);
	uart_write_ch('0' + pin);
	uart_write_ch(' ');
	if (!get_pin_is_output(port, pin)) {
		uart_write_ch('I');
		uart_write_ch('0' + get_pin_input(port, pin));
	} else {
		uart_write_ch('O');
		uart_write_ch('0' + get_pin_output(port, pin));
	}
	uart_write_ch(' ');
	uart_write_ch('C');
	uart_write_ch('1');
	uart_write_ch(' ');
	uart_write_ch('0' + get_pin_cr1(port, pin));
	uart_write_ch(' ');
	uart_write_ch('C');
	uart_write_ch('2');
	uart_write_ch(' ');
	uart_write_ch('0' + get_pin_cr2(port, pin));
	uart_write_ch('\r');
	uart_write_ch('\n');
}

void display_all_pins(void)
{
	uint8_t port;
	uint8_t pin;

	for (port = 'A'; port <= 'D'; port++) {
		for (pin = 0; pin <= 7; pin++) {
			display_pin_state(port, pin);
		}
	}
}

void uart_process_char(uint8_t ch)
{
	switch (ch) {
		case '\n':
		case '\r':
			display_pin_state(port, pin);
			break;

		case '?':
			display_all_pins();
			break;

		case 'A':
		case 'B':
		case 'C':
		case 'D':
			port = ch;
			display_pin_state(port, pin);
			break;
		case 'a':
		case 'b':
		case 'c':
		case 'd':
			port = 'A' + ch - 'a';
			display_pin_state(port, pin);
			break;

		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			pin = ch - '0';
			display_pin_state(port, pin);
			break;

		case 'I':
		case 'i':
			pin_turn_off(port, pin, TYPE_DDR);
			display_pin_state(port, pin);
			break;
		case 'O':
		case 'o':
			pin_turn_on(port, pin, TYPE_DDR);
			display_pin_state(port, pin);
			break;

		case 'H':
		case 'h':
			pin_turn_on(port, pin, TYPE_ODR);
			display_pin_state(port, pin);
			break;
		case 'L':
		case 'l':
			pin_turn_off(port, pin, TYPE_ODR);
			display_pin_state(port, pin);
			break;

		case 'Q':
		case 'q':
			pin_turn_on(port, pin, TYPE_CR1);
			display_pin_state(port, pin);
			break;
		case 'W':
		case 'w':
			pin_turn_off(port, pin, TYPE_CR1);
			display_pin_state(port, pin);
			break;

		case 'Z':
		case 'z':
			pin_turn_on(port, pin, TYPE_CR2);
			display_pin_state(port, pin);
			break;
		case 'X':
		case 'x':
			pin_turn_off(port, pin, TYPE_CR2);
			display_pin_state(port, pin);
			break;

		case 'S':
			scan = 1;
			uart_write_ch('S');
			uart_write_ch('\r');
			uart_write_ch('\n');
			break;

		case 's':
			scan = 0;
			uart_write_ch('s');
			uart_write_ch('\r');
			uart_write_ch('\n');
			break;

		case 0: // Do nothing if there is no input
			break;

		default:
			uart_write_ch('X');
			uart_write_ch('X');
			uart_write_ch('X');
			uart_write_ch(' ');
			display_pin_state(port, pin);
			break;
	}
}

uint8_t process_input(uint8_t port, uint8_t mask, uint8_t state, uint8_t last_state)
{
	uint8_t pin;

	if (state == last_state)
		return state;

	for (pin = 0; pin < 8; pin++) {
		uint8_t bit = 1 << pin;
		uint8_t cur_pin = (state & bit) ? 1 : 0;
		uint8_t last_pin = (last_state & bit) ? 1 : 0;

		if ((mask & bit) == 1) // Output bit, do not read
			continue;

		if (port == 'D') {
			if (pin == 5 || pin == 6)
				continue; // UART pins, dont sample
		} else if (port == 'A') {
			if (pin < 1 || pin > 3)
				continue; // Skip pin A5, it toggles all the time and cant be changed from input
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

void scan_inputs(void)
{
	last_pa_idr = process_input('A', PA_DDR, PA_IDR, last_pa_idr);
	last_pb_idr = process_input('B', PB_DDR, PB_IDR, last_pb_idr);
	last_pc_idr = process_input('C', PC_DDR, PC_IDR, last_pc_idr);
	last_pd_idr = process_input('D', PD_DDR, PD_IDR, last_pd_idr);
}


int main()
{
	uint8_t ch;
	scan = 1;

	clk_init();
	readbuf_init();
	uart_init();

	input_configure();

	port = 'A';
	pin = 0;

	do {
		if (readbuf_available())
			ch = readbuf_read();
		else
			ch = uart_read_ch_non_block();
		uart_process_char(ch);
		if (scan)
			scan_inputs();
	} while (1);
}
