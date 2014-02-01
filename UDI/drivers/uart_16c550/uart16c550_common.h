#ifndef _UART16C550_COMMON_H_
#define _UART16C550_COMMON_H_

enum {
	UART_OPS_DEV = 1,
	UART_OPS_IRQ,
	UART_OPS_GIO,
};

enum {
	UART_CB_BUS_BIND = 1,
	UART_CB_INTR,
	UART_CB_INTR_EVENT,
};

enum {
	PIO_RESET,
	PIO_TX,
	PIO_RX,
	N_PIO
};

typedef struct {
	udi_cb_t	*active_cb;
	struct {
		udi_index_t	pio_index;
	} init;
	
	udi_pio_handle_t	pio_handles[N_PIO];
	udi_channel_t	interrupt_channel;
	udi_buf_t	*rx_buffer;
} rdata_t;

// === MACROS ===
/* Copied from http://projectudi.cvs.sourceforge.net/viewvc/projectudi/udiref/driver/udi_dpt/udi_dpt.h */
#define DPT_SET_ATTR_BOOLEAN(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_BOOLEAN; \
		(attr)->attr_length = sizeof(udi_boolean_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR32(attr, name, val)	\
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_UBIT32; \
		(attr)->attr_length = sizeof(udi_ubit32_t); \
		UDI_ATTR32_SET((attr)->attr_value, (val))

#define DPT_SET_ATTR_ARRAY8(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_ARRAY8; \
		(attr)->attr_length = (len); \
		udi_memcpy((attr)->attr_value, (val), (len))

#define DPT_SET_ATTR_STRING(attr, name, val, len) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = (len); \
		udi_strncpy_rtrim((char *)(attr)->attr_value, (val), (len))
#define NE2K_SET_ATTR_STRFMT(attr, name, maxlen, fmt, v...) \
		udi_strcpy((attr)->attr_name, (name)); \
		(attr)->attr_type = UDI_ATTR_STRING; \
		(attr)->attr_length = udi_snprintf((char *)(attr)->attr_value, (maxlen), (fmt) ,## v )


#endif
