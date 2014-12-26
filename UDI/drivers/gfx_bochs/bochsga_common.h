/*
 * UDI Bochs Graphics Driver
 * By John Hodge (thePowersGang)
 *
 * bochsga_common.c
 * - Common definitions
 */
#ifndef _BOCHSGA_COMMON_H_
#define _BOCHSGA_COMMON_H_

/**
 * Definitions to match udiprops.txt
 * \{
 */
#define BOCHSGA_META_BUS	1
#define BOCHSGA_META_GFX	2

#define BOCHSGA_OPS_DEV	1
#define BOCHSGA_OPS_GFX	2

#define BOCHSGA_CB_BUS_BIND	1
#define BOCHSGA_CB_GFX_BIND	2
#define BOCHSGA_CB_GFX_STATE	3
#define BOCHSGA_CB_GFX_RANGE	4
#define BOCHSGA_CB_GFX_COMMAND	5

#define BOCHSGA_MSGNUM_PROPUNK	1001
#define BOCHSGA_MSGNUM_BUFUNK	1002
/**
 * \}
 */

#include "bochsga_pio.h"

typedef struct {
	udi_ubit32_t	width;
	udi_ubit32_t	height;
	udi_index_t	bitdepth;
} engine_t;

#define N_ENGINES	1

/**
 * Region data
 */
typedef struct
{
	udi_cb_t	*active_cb;
	struct {
		udi_index_t	pio_index;
	} init;

	udi_pio_handle_t	pio_handles[N_PIO];
	
	udi_boolean_t	output_enable;
	struct {
		udi_ubit32_t	width;
		udi_ubit32_t	height;
		udi_ubit8_t	bitdepth;
		udi_index_t	engine;
	} outputstate;
	struct {
		udi_ubit32_t	max_width;	// 1024 or 1280
		udi_ubit32_t	max_height;	// 768 or 1024
	} limits;
	
	engine_t	engines[N_ENGINES];
} rdata_t;

#define BOCHSGA_MIN_WIDTH	360
#define BOCHSGA_MIN_HEIGHT	240

#endif

