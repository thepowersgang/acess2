#define PIO_op_RI(op, reg, sz, val)	{UDI_PIO_##op+UDI_PIO_DIRECT+UDI_PIO_##reg, UDI_PIO_##sz##BYTE, val}
#define PIO_MOV_RI1(reg, val)	PIO_op_RI(LOAD_IMM, reg, 2, val)
#define PIO_OUT_RI1(reg, ofs)	PIO_op_RI(OUT, reg, 1, ofs)
#define PIO_IN_RI1(reg, ofs)	PIO_op_RI(IN, reg, 1, ofs)

//
// Ne2000 reset operation (reads MAC address too)
// 
udi_pio_trans_t	ne2k_pio_reset[] = {
	// - Reset card
	PIO_IN_RI1(R0, NE2K_REG_RESET),
	PIO_OUT_RI1(R0, NE2K_REG_RESET),
	// While ISR bit 7 is unset, spin
	{UDI_PIO_LABEL, 0, 1},
		PIO_IN_RI1(R0, NE2K_REG_ISR),
		{UDI_PIO_AND_IMM+UDI_PIO_R0, UDI_PIO_1BYTE, 0x80},
	{UDI_PIO_CSKIP+UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NZ},
	{UDI_PIO_BRANCH, 0, 1},
	// ISR = 0x80 [Clear reset]
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// - Init pass 1
	// CMD = 0x40|0x21 [Page1, NoDMA, Stop]
	PIO_MOV_RI1(R0, 0x40|0x21),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// CURR = First RX page
	PIO_MOV_RI1(R0, NE2K_RX_FIRST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_CURR),
	// CMD = 0x21 [Page0, NoDMA, Stop]
	PIO_MOV_RI1(R0, 0x21),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// DCR = ? [WORD, ...]
	PIO_MOV_RI1(R0, 0x49),
	PIO_OUT_RI1(R0, NE2K_REG_DCR),
	// IMR = 0 [Disable all]
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, NE2K_REG_IMR),
	// ISR = 0xFF [ACK all]
	PIO_MOV_RI1(R0, 0xFF),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// RCR = 0x20 [Monitor]
	PIO_MOV_RI1(R0, 0x20),
	PIO_OUT_RI1(R0, NE2K_REG_RCR),
	// TCR = 0x02 [TX Off, Loopback]
	PIO_MOV_RI1(R0, 0x02),
	PIO_OUT_RI1(R0, NE2K_REG_TCR),
	// - Read MAC address from EEPROM (24 bytes from 0)
	PIO_MOV_RI1(R0, 0),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R0, NE2K_REG_RSAR0),
	PIO_OUT_RI1(R1, NE2K_REG_RSAR1),
	PIO_MOV_RI1(R0, 6*4),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR0),
	PIO_OUT_RI1(R1, NE2K_REG_RBCR1),
	// CMD = 0x0A [Start remote DMA]
	PIO_MOV_RI1(R0, 0x0A),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// Read MAC address
	PIO_MOV_RI1(R0, 0),	// - Buffer offset (incremented by 1 each iteration)
	PIO_MOV_RI1(R1, NE2K_REG_MEM),	// - Reg offset (no increment)
	PIO_MOV_RI1(R2, 6),	// - Six iterations
	{UDI_PIO_REP_IN_IND, UDI_PIO_1BYTE,
		UDI_PIO_REP_ARGS(UDI_PIO_MEM, UDI_PIO_R0, 1, UDI_PIO_R1, 0, UDI_PIO_R2)},
	// End
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};
//
// Enable the card
//
udi_pio_trans_t	ne2k_pio_enable[] = {
	// - Setup
	// PSTART = First RX page [Receive area start]
	PIO_MOV_RI1(R0, NE2K_RX_FIRST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_PSTART),
	// BNRY = Last RX page - 1 [???]
	PIO_MOV_RI1(R0, NE2K_RX_LAST_PG-1),
	PIO_OUT_RI1(R0, NE2K_REG_BNRY),
	// PSTOP = Last RX page [???]
	PIO_MOV_RI1(R0, NE2K_RX_LAST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_PSTOP),
	// CMD = 0x22 [NoDMA, Start]
	PIO_MOV_RI1(R0, 0x22),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// RCR = 0x0F [Wrap, Promisc]
	PIO_MOV_RI1(R0, 0x0F),
	PIO_OUT_RI1(R0, NE2K_REG_RCR),
	// TCR = 0x00 [Normal]
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, NE2K_REG_TCR),
	// TPSR = 0x40 [TX Start]
	PIO_MOV_RI1(R0, 0x40),
	PIO_OUT_RI1(R0, NE2K_REG_TPSR),
	// End
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0}
};
//
// Read a packet
// - Expects an output buffer
//
udi_pio_trans_t ne2k_pio_rx[] = {
	// Get current page into R7
	PIO_MOV_RI1(R0, 0),
	{UDI_PIO_LOAD|UDI_PIO_MEM|UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	// Read header into regs, then data into buffer
	// - CMD = 0x22
	PIO_MOV_RI1(R0, 0x22),	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// - Clear RDMA flag
	PIO_MOV_RI1(R0, 0x40),	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// - Set up transaction for 1 page from CurRX
	PIO_MOV_RI1(R0, 1),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R7, NE2K_REG_RSAR0),	// Current RX page
	PIO_OUT_RI1(R1, NE2K_REG_RSAR1),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR0),
	PIO_OUT_RI1(R1, NE2K_REG_RBCR1),
	// - Start read
	PIO_MOV_RI1(R0, 0x0A),	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	{UDI_PIO_DELAY, 0, 0},
	//  > Header to registers
	{UDI_PIO_IN|UDI_PIO_DIRECT|UDI_PIO_R6, UDI_PIO_2BYTE, NE2K_REG_MEM},	// Status,NextPacketPage
	{UDI_PIO_IN|UDI_PIO_DIRECT|UDI_PIO_R5, UDI_PIO_2BYTE, NE2K_REG_MEM},	// Length (bytes)
	//  > Data to buffer (126 words)
	PIO_MOV_RI1(R4, 0),	// - Buffer offset (incremented by 1 each iteration)
	PIO_MOV_RI1(R1, NE2K_REG_MEM),	// - Reg offset (no increment)
	PIO_MOV_RI1(R2, 256/2-2),	// - Six iterations
	{UDI_PIO_REP_IN_IND, UDI_PIO_2BYTE,
		UDI_PIO_REP_ARGS(UDI_PIO_BUF, UDI_PIO_R4, 2, UDI_PIO_R1, 0, UDI_PIO_R2)},
	// - Subtract 256-4 from length, if <=0 we've grabbed the entire packet
	{UDI_PIO_SUB|UDI_PIO_R5, UDI_PIO_2BYTE, 256-4},
	{UDI_PIO_LABEL, 0, 2},
	{UDI_PIO_CSKIP|UDI_PIO_R5, UDI_PIO_2BYTE, UDI_PIO_NZ},
	{UDI_PIO_BRANCH, 0, 1},	// 1: End if ==0
	{UDI_PIO_CSKIP|UDI_PIO_R5, UDI_PIO_2BYTE, UDI_PIO_NNEG},
	{UDI_PIO_BRANCH, 0, 1},	// 1: End if <0
	// - Read pages until all of packet RXd
	{UDI_PIO_ADD|UDI_PIO_DIRECT|UDI_PIO_R7, UDI_PIO_1BYTE, 1},
	{UDI_PIO_STORE|UDI_PIO_DIRECT|UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},
	{UDI_PIO_SUB|UDI_PIO_R0, UDI_PIO_1BYTE, NE2K_RX_LAST_PG+1},
	{UDI_PIO_CSKIP|UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_NZ},	// if R7-RX_LAST == 0
	{UDI_PIO_LOAD_IMM|UDI_PIO_R7, UDI_PIO_2BYTE, NE2K_RX_FIRST_PG},	// R7 = RX_FIRST
	//  > Transaction start
	PIO_MOV_RI1(R0, 1),
	PIO_MOV_RI1(R1, 0),
	PIO_OUT_RI1(R7, NE2K_REG_RSAR0),	// Current RX page
	PIO_OUT_RI1(R1, NE2K_REG_RSAR1),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR0),
	PIO_OUT_RI1(R1, NE2K_REG_RBCR1),
	PIO_MOV_RI1(R0, 0x0A),	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	{UDI_PIO_DELAY, 0, 0},
	//  > Data to buffer (128 words)
	// buffer offset maintained in R4
	PIO_MOV_RI1(R1, NE2K_REG_MEM),	// - Reg offset (no increment)
	PIO_MOV_RI1(R2, 256/2),	// - Six iterations
	{UDI_PIO_REP_IN_IND, UDI_PIO_2BYTE,
		UDI_PIO_REP_ARGS(UDI_PIO_BUF, UDI_PIO_R4, 2, UDI_PIO_R1, 0, UDI_PIO_R2)},
	// - Jump to length check
	{UDI_PIO_SUB|UDI_PIO_R5, UDI_PIO_2BYTE, 256},
	{UDI_PIO_BRANCH, 0, 2},	// 2: Check against length
	
	// Cleanup
	{UDI_PIO_LABEL, 0, 1},
	// - Update next RX page, return status
	{UDI_PIO_OUT|UDI_PIO_DIRECT|UDI_PIO_R7, UDI_PIO_1BYTE, NE2K_REG_BNRY},
	{UDI_PIO_STORE|UDI_PIO_DIRECT|UDI_PIO_R7, UDI_PIO_2BYTE, UDI_PIO_R6},	// Status,Next
	{UDI_PIO_SHIFT_RIGHT|UDI_PIO_DIRECT|UDI_PIO_R7, UDI_PIO_2BYTE, 8},	// Next
	PIO_MOV_RI1(R0, 0),
	{UDI_PIO_STORE|UDI_PIO_MEM|UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_R7},	// Store to mem[0]
	
	{UDI_PIO_END, UDI_PIO_1BYTE, UDI_PIO_R6}	// Status
};
//
//
//
udi_pio_trans_t	ne2k_pio_irqack[] = {
	// 0: Enable interrupts
	// IMR = 0x3F []
	PIO_MOV_RI1(R0, 0x3F),
	PIO_OUT_RI1(R0, NE2K_REG_IMR),
	// ISR = 0xFF [ACK all]
	PIO_MOV_RI1(R0, 0xFF),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
	// 1: Normal
	{UDI_PIO_LABEL, 0, 1},
	PIO_IN_RI1(R0, NE2K_REG_ISR),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	{UDI_PIO_CSKIP|UDI_PIO_R0, UDI_PIO_1BYTE, UDI_PIO_Z},	// if R0!=0
	{UDI_PIO_END, UDI_PIO_2BYTE, UDI_PIO_R0},
	// - No IRQ, quiet quit
	PIO_MOV_RI1(R0, UDI_INTR_NO_EVENT),
	PIO_MOV_RI1(R1, 0),	// scratch offset
	{UDI_PIO_STORE|UDI_PIO_SCRATCH|UDI_PIO_R1, UDI_PIO_1BYTE, UDI_PIO_R0},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
	// 2: Overrun
	{UDI_PIO_LABEL, 0, 2},
	// 3: Overrun irqs
	{UDI_PIO_LABEL, 0, 3},
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
};
//
//
//
udi_pio_trans_t ne2k_pio_tx[] = {
	// Switch to page 0
	// CMD = 0x40|0x21 [Page1, NoDMA, Stop]
	PIO_MOV_RI1(R0, 0x40|0x21),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// Clear RDMA
	PIO_MOV_RI1(R0, 0x40),
	PIO_OUT_RI1(R0, NE2K_REG_ISR),
	// Send size (TBCR, RBCR)
	PIO_MOV_RI1(R1, 0x00),
	{UDI_PIO_LOAD|UDI_PIO_MEM|UDI_PIO_R0, UDI_PIO_2BYTE, UDI_PIO_R1},
	PIO_OUT_RI1(R0, NE2K_REG_TBCR0),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR0),
	{UDI_PIO_SHIFT_RIGHT|UDI_PIO_R0, UDI_PIO_2BYTE, 8},
	PIO_OUT_RI1(R0, NE2K_REG_TBCR1),
	PIO_OUT_RI1(R0, NE2K_REG_RBCR1),
	// Set up transfer
	PIO_MOV_RI1(R0, 0x00),
	PIO_OUT_RI1(R0, NE2K_REG_RSAR0),
	PIO_MOV_RI1(R0, NE2K_TX_FIRST_PG),	// single TX memory range
	PIO_OUT_RI1(R0, NE2K_REG_RSAR1),
	// Start
	PIO_MOV_RI1(R0, 0x12),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	// Send data
	PIO_MOV_RI1(R0, 0),	// - Buffer offset (increment by 2)
	PIO_MOV_RI1(R1, NE2K_REG_MEM),	// - Reg offset (no increment)
	PIO_MOV_RI1(R2, 256/2),	// - 128 iterations
	{UDI_PIO_REP_OUT_IND, UDI_PIO_2BYTE,
		UDI_PIO_REP_ARGS(UDI_PIO_BUF, UDI_PIO_R0, 2, UDI_PIO_R1, 0, UDI_PIO_R2)},
	// Wait for completion (TODO: IRQ quit and wait for IRQ)
	// Request send
	PIO_MOV_RI1(R0, NE2K_TX_FIRST_PG),
	PIO_OUT_RI1(R0, NE2K_REG_TPSR),
	PIO_MOV_RI1(R0, 0x10|0x04|0x02),
	PIO_OUT_RI1(R0, NE2K_REG_CMD),
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
};

struct {
	udi_pio_trans_t	*trans_list;
	udi_ubit16_t	list_length;
	udi_ubit16_t	pio_attributes;
} ne2k_pio_ops[] = {
	[NE2K_PIO_RESET]  = {ne2k_pio_reset, ARRAY_SIZEOF(ne2k_pio_reset), 0},
	[NE2K_PIO_ENABLE] = {ne2k_pio_enable, ARRAY_SIZEOF(ne2k_pio_enable), 0},
	[NE2K_PIO_RX]     = {ne2k_pio_rx, ARRAY_SIZEOF(ne2k_pio_rx), 0},
	[NE2K_PIO_IRQACK] = {ne2k_pio_irqack, ARRAY_SIZEOF(ne2k_pio_irqack), 0},
	[NE2K_PIO_TX]     = {ne2k_pio_tx, ARRAY_SIZEOF(ne2k_pio_tx), 0},
};
const int NE2K_NUM_PIO_OPS = ARRAY_SIZEOF(ne2k_pio_ops);
