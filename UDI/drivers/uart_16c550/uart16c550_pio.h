#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI2(reg, val)	PIO_op_RI(LOAD_IMM, reg, 2, val)
#define PIO_ADD_RI2(reg, val)	PIO_op_RI(ADD, reg, 2, val)
#define PIO_SUB_RI2(reg, val)	PIO_op_RI(SUB, reg, 2, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)

//
// Reset
// 
udi_pio_trans_t	uart_pio_reset[] = {
	// TODO: Programmable baud rate
	//PIO_MOV_RI2(R0, 0x3F8),	// TODO: programmable port number
	PIO_ADD_RI2(R0, 1),
	PIO_OUT_RI1(R0, 0x00),	// +1 = 0x00 - Disable interrupts
	PIO_ADD_RI2(R0, 2),
	PIO_OUT_RI1(R0, 0x80),	// +3 = 0x80 - Enable DLAB
	PIO_SUB_RI2(R0, 3),
	PIO_OUT_RI1(R0, 0x01),	// +0 = 0x01 - Divisor low	(115200 baud)
	PIO_ADD_RI2(R0, 1),
	PIO_OUT_RI1(R0, 0x00),	// +1 = 0x00 - Divisor high
	PIO_ADD_RI2(R0, 2),
	PIO_OUT_RI1(R0, 0x03),	// +3 = 0x03 - 8n1
	PIO_SUB_RI2(R0, 1),
	PIO_OUT_RI1(R0, 0xC7),	// +2 = 0xC7 - Clear FIFO, 14-byte threshold
	PIO_ADD_RI2(R0, 2),
	PIO_OUT_RI1(R0, 0x0B),	// +4 = 0x0B - IRQs enabled, RTS/DSR set
	PIO_SUB_RI2(R0, 3),
	PIO_OUT_RI1(R0, 0x0B),	// +1 = 0x05 - Enable ERBFI (Rx Full), ELSI (Line Status)
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};
//
// Transmit
//
udi_pio_trans_t	uart_pio_tx[] = {
	// while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	// outb(SERIAL_PORT, ch);
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};
//
// Recieve (interrupt)
//
udi_pio_trans_t	uart_pio_rx[] = {
	// if( (inb(SERIAL_PORT+5) & 0x01) == 0 )
	//	return -1;
	// return inb(SERIAL_PORT); 
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};

#define ARRAY_SIZEOF(arr)	(sizeof(arr)/sizeof(arr[0]))

struct {
	udi_pio_trans_t	*trans_list;
	udi_ubit16_t	list_length;
	udi_ubit16_t	pio_attributes;
} uart_pio_ops[] = {
	[PIO_RESET]  = {uart_pio_reset, ARRAY_SIZEOF(uart_pio_reset), 0},
	[PIO_TX]     = {uart_pio_tx, ARRAY_SIZEOF(uart_pio_tx), 0},
	[PIO_RX]     = {uart_pio_rx, ARRAY_SIZEOF(uart_pio_rx), 0},
};
//const int UART_NUM_PIO_OPS = ARRAY_SIZEOF(uart_pio_ops);

