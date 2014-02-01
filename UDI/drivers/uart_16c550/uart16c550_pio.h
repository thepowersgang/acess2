#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI1(reg, val)	PIO_op_RI(LOAD_IMM, reg, 2, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)

//
// Reset
// 
udi_pio_trans_t	uart_pio_reset[] = {
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};
//
// Transmit
//
udi_pio_trans_t	uart_pio_tx[] = {
	{UDI_PIO_END, UDI_PIO_2BYTE, 0}
};
//
// Recieve (interrupt)
//
udi_pio_trans_t	uart_pio_rx[] = {
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

