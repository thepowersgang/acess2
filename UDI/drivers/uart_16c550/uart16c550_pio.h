#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI2(reg, val)	PIO_op_RI(LOAD_IMM, reg, 2, val)
#define PIO_MOV_RI1(reg, val)	PIO_op_RI(LOAD_IMM, reg, 1, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)

//
// Reset
// 
udi_pio_trans_t	uart_pio_reset[] = {
	// TODO: Programmable baud rate
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, 1),	// +1 = 0x00 - Disable interrupts
	PIO_MOV_RI1(R0, 0x80),
	PIO_OUT_RI1(R0, 3),	// +3 = 0x80 - Enable DLAB
	PIO_MOV_RI1(R0, 0x01),
	PIO_OUT_RI1(R0, 0),	// +0 = 0x01 - Divisor low	(115200 baud)
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, 1),	// +1 = 0x00 - Divisor high
	PIO_MOV_RI1(R0, 0x03),
	PIO_OUT_RI1(R0, 3),	// +3 = 0x03 - 8n1
	PIO_MOV_RI1(R0, 0xC7),
	PIO_OUT_RI1(R0, 2),	// +2 = 0xC7 - Clear FIFO, 14-byte threshold
	PIO_MOV_RI1(R0, 0x0B),
	PIO_OUT_RI1(R0, 4),	// +4 = 0x0B - IRQs enabled, RTS/DSR set
	PIO_MOV_RI1(R0, 0x0B),
	PIO_OUT_RI1(R0, 1),	// +1 = 0x05 - Enable ERBFI (Rx Full), ELSI (Line Status)
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};
//
// Transmit
//
udi_pio_trans_t	uart_pio_tx[] = {
	// while( (inb(SERIAL_PORT + 5) & 0x20) == 0 );
	// outb(SERIAL_PORT, ch);
	
	// R0: Temp
	// R1: Byte to write
	// R2: Buffer size
	// R3: Position
	{UDI_PIO_LOAD_IMM+UDI_PIO_DIRECT+UDI_PIO_R0, UDI_PIO_2BYTE, 0},
	{UDI_PIO_LOAD+UDI_PIO_MEM+UDI_PIO_R0, UDI_PIO_4BYTE, UDI_PIO_R2},
	
	{UDI_PIO_LABEL, 0, 1},	// Loop top
	
	PIO_op_RI(LOAD, R0, 2, UDI_PIO_R3),
	PIO_op_RI(SUB, R0, 2, UDI_PIO_R2),
	{UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 3},
	
	// Load byte into R1 (and increment R3)
	{UDI_PIO_LOAD+UDI_PIO_BUF+UDI_PIO_R3, UDI_PIO_1BYTE, UDI_PIO_R1},
	PIO_op_RI(ADD_IMM, R3, 2, 1),
	
	// TX single byte from R1
	// - Wait for FIFO to clear
	{UDI_PIO_LABEL, 0, 2},
	PIO_IN_RI1(R0, 5),
	{UDI_PIO_AND_IMM+UDI_PIO_DIRECT+UDI_PIO_DIRECT, UDI_PIO_1BYTE, 0x20},
	{UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},
	{UDI_PIO_BRANCH, 0, 1},
	// - TX
	PIO_OUT_RI1(R1, 0),
	{UDI_PIO_BRANCH, 0, 2},
	
	// Done
	{UDI_PIO_LABEL, 0, 3},
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

