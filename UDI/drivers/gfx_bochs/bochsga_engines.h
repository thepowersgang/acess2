/*
 * 
 */

#define BOCHSGA_ENGINE_PROP_BUFFER	(UDI_GFX_PROP_CUSTOM+0)

/* === CONSTANTS === */
const gfxhelpers_op_t	bochsga_engine_ops_8bpp[] = {
};
const gfxhelpers_op_t	bochsga_engine_ops_32bpp[] = {
	{UDI_GFX_OPERATOR_RGB,    1,  2,  3},	/* #0 Output RGB from ops #1,#2,#3 */
	{UDI_GFX_OPERATOR_SEG,    4, 16,  8},	/* #1 Extract 8 bits from bit 16 of #4 */
	{UDI_GFX_OPERATOR_SEG,    4,  8,  8},	/* #2 8 bits from ofs 8 of #4 */
	{UDI_GFX_OPERATOR_SEG,    4,  0,  8},	/* #3 8 bits from ofs 0 of #4 */
	{UDI_GFX_OPERATOR_BUFFER, 5,  6, 32},	/* #4 32 bits from buffer #5 ofs #6 */
	{UDI_GFX_OPERATOR_ATTR,   0, BOCHSGA_ENGINE_PROP_BUFFER, 0},	/* #5 Buffer index */
	{UDI_GFX_OPERATOR_MAD,    7,  8,  9},	/* #6 Get offset (#8 * #7 + #9) */
	{UDI_GFX_OPERATOR_ATTR,   0, UDI_GFX_PROP_SOURCE_WIDTH, 0},	/* #7 Read buffer width */
	{UDI_GFX_OPERATOR_Y,      0,  0,  0},	/* #8 Y coordinate */
	{UDI_GFX_OPERATOR_X,      0,  0,  0}	/* #9 X coordinate */
};

typedef struct {
	udi_ubit8_t	bitdepth;
	gfxhelpers_op_map_t	op_map;
} engine_static_t;

const engine_static_t	bochsga_engine_defs[] = {
	{.bitdepth = 8, .op_map = {ARRAY_COUNT(bochsga_engine_ops_8bpp), bochsga_engine_ops_8bpp}},
	{.bitdepth = 16},
	{.bitdepth = 24},
	{.bitdepth = 32, .op_map = {ARRAY_COUNT(bochsga_engine_ops_8bpp), bochsga_engine_ops_32bpp}},
};
#define N_ENGINES	ARRAY_COUNT(bochsga_engine_defs)
