/*
 * TODO
 */
#ifndef _BOCHSGA_PIO_H_
#define _BOCHSGA_PIO_H_

udi_pio_trans_t	bochsga_pio_enable[] = {
	{UDI_PIO_END_IMM, UDI_PIO_2BYTE, 0},
	};

enum {
	BOCHSGA_PIO_ENABLE,
};

const struct s_pio_ops	bochsga_pio_ops[] = {
	[BOCHSGA_PIO_ENABLE] = UDIH_PIO_OPS_ENTRY(bochsga_pio_enable, 0, UDI_PCI_BAR_2, 0x400, 0xB*2),
//	UDIH_PIO_OPS_ENTRY(bochsga_pio_enable, 0, UDI_PCI_BAR_2, 0x400, 0xB*2),
	};
#define N_PIO	(sizeof(bochsga_pio_ops)/sizeof(struct s_pio_ops))

#endif

