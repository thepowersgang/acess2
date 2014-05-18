/**
 * Summary: udi_gfx.h
 * Contains the graphics metalanguage interface details
 *
 * Author:
 *     Marcel Sondaar
 *
 * License:
 *     <Public Domain>
 *
 * Source:
 *     https://www.d-rift.nl/combuster/mos3/?p=viewsource&file=/include/common/udi_gfx.h
 */
 
// note that the specification, and thus, the contents of this file is not fixed.
 
#ifndef __UDI_GFX_H__
#define __UDI_GFX_H__
 
#include <udi.h>
 
#ifndef UDI_GFX_VERSION
#error "UDI_GFX_VERSION not defined."
#elif UDI_GFX_VERSION != 0x101
#error "UDI_GFX_VERSION not supported."
#endif
 
/**
 * Enumeration: UDI_GFX_PROP
 * Lists the various UDI properties
 */
 
// General state properties
/* Constant: UDI_GFX_PROP_ENABLE
 *
 * Valid values:
 *     0 - disabled
 *     1 - enabled
 *
 * Ranges:
 *     Hardwired 1, or 0-1
 *
 * The connector or engine is enabled (nonzero) or disabled (zero). A disabled
 * engine forwards all data from the previous stage unmodified. A disabled
 * connector does not send any data over the connector. Drivers may power down
 * the physical counterparts of disabled components to preserve power, and users
 * should expect delays when enabling connectors or components representing
 * framebuffers. Disabling is however not recommended for sprite layers, which
 * may repeatedly be enabled and disabled. A disabled component can still have
 * its state changed, and the driver must complete all commands and other state
 * changes as expected, regardless of disabled or power state. The valid ranges
 * reported for this property may be 1 (always enabled) or 0-1 (either enabled
 * or disabled).
 */
#define UDI_GFX_PROP_ENABLE 0
/* Constant: UDI_GFX_PROP_INPUT
 *
 * Valid values:
 *     Any valid engine ID, provided no dependency cycles are created, or -1
 *
 * Ranges:
 *     Any non-empty set of valid values. Often hardwired.
 *
 * Points to the engine that is processed before this unit. In the case of a 
 * connector, it points to the last engine in a pipeline, and each engine points 
 * to the next engine in the sequence. A value of -1 indicates a source that 
 * only yields black pixels. Implementations must not allow cyclic structures. 
 * Changing this value may reallocate resources, and engines that are no longer 
 * referenced may lose their data (but not their state) when it is not part of 
 * any pipeline. If preservation is required, the ENABLE state should be used
 * instead. Valid ranges includes one or more from the list of engines and -1 
 * combined. In most cases, this property can not be modified.
 */
#define UDI_GFX_PROP_INPUT 1
/* Constant: UDI_GFX_PROP_WIDTH
 *
 * Valid values:
 *     Any non-zero positive number.
 *
 * Ranges:
 *     Contains at least one valid value. Often only multiples of UNIT_WIDTH
 *     or a power of two are allowed. May be hardwired.
 *
 * Contains the amount of pixels in the horizontal direction. For connectors, 
 * this is the amount of data pixels rendered horizontally. For engines, this 
 * is the width in pixels of the image. Pixels requested from an engine outside 
 * the range (0..width-1) are defined according to the <UDI_GFX_PROP_CLIP> 
 * property. In some cases, hardware may support only fixed combinations of 
 * width and height. In such cases, changing the width will also change the 
 * height to a corresponding valid number. Valid ranges include any values
 * strictly above zero. For connectors, expect large continuous ranges, large
 * ranges with a certain modulus, a limited number of fixed values, or a
 * constant value.
 */
#define UDI_GFX_PROP_WIDTH 2
/* Constant: UDI_GFX_PROP_HEIGHT
 *
 * Valid values:
 *     Any non-zero positive number.
 *
 * Ranges:
 *     Contains at least one valid value. Often only multiples of UNIT_HEIGHT
 *     or a power of two are allowed. May be hardwired.
 *
 * Contains the amount of pixels in the vertical direction. Functions similar
 * to the width property, but changing it will not alter the width property,
 * and it's range at any time contains the valid range for the currently
 * selected width.
 */
#define UDI_GFX_PROP_HEIGHT 3
 
/* Constant: UDI_GFX_PROP_CUSTOM
 * The first property index of the driver's custom range. These are not assigned
 * directly assigned by the UDI specification, but may be specified in the
 * operator tree.
 */
#define UDI_GFX_PROP_CUSTOM 1024
 
// engine properties
 
/* Constant: UDI_GFX_PROP_CLIP
 *
 * Valid values:
 *     0 - points outside width x height are passed from next stage
 *     1 - the engine's contents is tiled with size width x height
 *     2 - points outside the width overflow into the y coordinate
 *     3 - points outside the height overflow into the x coordinate
 *
 * Ranges:
 *     Hardwired zero for connectors. Any non-empty subset for engines, usually
 *     hardwired.
 *
 * For engines, contains the behaviour for pixels requested outside the width
 * and height of the engine. Can be either 0 (pass from next stage), 1 (the
 * coordinates are wrapped modulus the height and width), 2 (the coordinates
 * overflow onto the next scanline horizontally, and wrap vertically), 3 (the
 * coordinates overflow onto the next column vertically, and wrap horizontally).
 * Valid ranges contain one or more of these options. For overlays and sprites,
 * a value 0 is common. For framebuffers, 2 is the most common value. For
 * connectors, this property is always 0 since they do not store pixel data
 */
#define UDI_GFX_PROP_CLIP 4
 
/* Constant: UDI_GFX_PROP_UNIT_WIDTH
 *
 * Valid values:
 *     Any non-zero positive value
 *
 * Ranges:
 *     Any non-empty set of valid values. May be hardwired to 1 for
 *     framebuffers, or a range of small values for hardware scaling, or any
 *     larger hardwired number or set for tiling engines.
 *
 * Tiles are used to indicate that the hardware groups sets of pixels and have
 * each group share certain properties, i.e. color or tile index, or share the
 * same chroma subsample with only a different intensity. If the engine has no
 * such grouping, or shares all properties over the entire contents, the value
 * of this property should be 1. Some tile examples include rescaling, where a
 * tile width of 2 indicates a pixel doubling in X direction, or in text mode
 * where a tile width of 8 or 9 corresponds with the width of common bitmap
 * fonts
 */
#define UDI_GFX_PROP_UNIT_WIDTH 5
 
/* Constant: UDI_GFX_PROP_UNIT_HEIGHT
 *
 * Valid values:
 *     Any non-zero positive value
 *
 * Ranges:
 *     Any non-empty set of valid values. May be hardwired to 1 for
 *     framebuffers, or a range of small values for hardware scaling, or any
 *     larger hardwired number or set for tiling engines.
 *
 * See <UDI_GFX_PROP_UNIT_WIDTH>, but for the Y direction. Common values are
 * 1-2 for framebuffers (doublescanning on or off), identical to the tile
 * width, or mostly independent.
 */
#define UDI_GFX_PROP_UNIT_HEIGHT 6
 
#define UDI_GFX_PROP_TRANSLATEX 7
#define UDI_GFX_PROP_TRANSLATEY 8
#define UDI_GFX_PROP_SOURCE_WIDTH 12
#define UDI_GFX_PROP_SOURCE_HEIGHT 13
#define UDI_GFX_PROP_GL_VERSION 14
#define UDI_GFX_PROP_GLES_VERSION 15
#define UDI_GFX_PROP_STATE_BLOCK 16
/**
 * Each engine consists of 1 or more operators
 */
#define UDI_GFX_PROP_OPERATOR_INDEX 17	//!< Index of operator to inspect/manipulate
#define UDI_GFX_PROP_OPERATOR_OPCODE 18	//!< Operation performed by operator
#define UDI_GFX_PROP_OPERATOR_ARG_1 19	//!< argument 1
#define UDI_GFX_PROP_OPERATOR_ARG_2 20	//!< argument 2
#define UDI_GFX_PROP_OPERATOR_ARG_3 21	//!< argument 3
#define UDI_GFX_PROP_COLOR_BITS 22
#define UDI_GFX_PROP_GL_TARGET 23
#define UDI_GFX_PROP_INPUTX 25
#define UDI_GFX_PROP_INPUTY 26
 
// properties for removal:
#define UDI_GFX_PROP_STORE_COUNT 24       // not generic
#define UDI_GFX_PROP_STORE_WIDTH 9        // not generic
#define UDI_GFX_PROP_STORE_HEIGHT 10      // not generic
#define UDI_GFX_PROP_STORE_BITS 11        // not generic
#define UDI_GFX_PROP_PALETTE 1024         // optional, can be derived from the operator tree
#define UDI_GFX_PROP_BUFFER 1025          // optional, can be derived from the operator tree
#define UDI_GFX_PROP_TILESHEET 1026       // optional, can be derived from the operator tree
 
// connector properties
#define UDI_GFX_PROP_SIGNAL 23
#define UDI_GFX_PROP_CONNECTOR_TYPE 24
#define UDI_GFX_PROP_VGA_H_FRONT_PORCH 25
#define UDI_GFX_PROP_VGA_H_BACK_PORCH 26
#define UDI_GFX_PROP_VGA_H_SYNC 27
#define UDI_GFX_PROP_VGA_V_FRONT_PORCH 28
#define UDI_GFX_PROP_VGA_V_BACK_PORCH 29
#define UDI_GFX_PROP_VGA_V_SYNC 30
#define UDI_GFX_PROP_DOT_CLOCK 31
#define UDI_GFX_PROP_VGA_H_SYNC_POL 32
#define UDI_GFX_PROP_VGA_V_SYNC_POL 33
 
/**
 * Enumeration: UDI_GFX_SIGNAL
 * Lists the various signal types
 */
#define UDI_GFX_SIGNAL_HIDDEN 0
#define UDI_GFX_SIGNAL_INTEGRATED 0
#define UDI_GFX_SIGNAL_RGBHV 1
#define UDI_GFX_SIGNAL_RGBS 2
#define UDI_GFX_SIGNAL_RGSB 3
#define UDI_GFX_SIGNAL_YPBPR 4
#define UDI_GFX_SIGNAL_DVID 5
#define UDI_GFX_SIGNAL_YUV 6
#define UDI_GFX_SIGNAL_YIQ 7
#define UDI_GFX_SIGNAL_Y_UV 8
#define UDI_GFX_SIGNAL_Y_IQ 9
#define UDI_GFX_SIGNAL_HDMI 10
#define UDI_GFX_SIGNAL_TEXT 11
#define UDI_GFX_SIGNAL_CUSTOM 12
 
/**
 * Enumeration: UDI_GFX_CONNECTOR
 * Lists the various external connectors
 */
#define UDI_GFX_CONNECTOR_HIDDEN 0
#define UDI_GFX_CONNECTOR_VGA 1
#define UDI_GFX_CONNECTOR_DVI 2
#define UDI_GFX_CONNECTOR_SVIDEO 3
#define UDI_GFX_CONNECTOR_COMPONENT 4
#define UDI_GFX_CONNECTOR_HDMI 5
#define UDI_GFX_CONNECTOR_RF 6
#define UDI_GFX_CONNECTOR_SCART 7
#define UDI_GFX_CONNECTOR_COMPOSITE 8
 
/**
 * Enumeration: UDI_GFX_OPERATOR
 * Lists the display output operator
 * 
 * a1/a2/a3 represents taking the output of a previous operation
 * v1/v2/v3 represents the literal value of that argument
 */
#define UDI_GFX_OPERATOR_RGB     0 // output = (color) red(a1) + green(a2) + blue(a3) (each component is UDI_GFX_PROP_COLOR_BITS
#define UDI_GFX_OPERATOR_YUV     1 // output = (color) Y(a1) + U(a2) + V(a3)
#define UDI_GFX_OPERATOR_YIQ     2 // output = (color) Y(a1) + I(a2) + Q(a3)
#define UDI_GFX_OPERATOR_I       3 // output = (color) intensity(a1)
#define UDI_GFX_OPERATOR_ALPHA   4 // output = (color) a1 + alpha(a2)
#define UDI_GFX_OPERATOR_ADD     5 // output = a1 + a2 + v3
#define UDI_GFX_OPERATOR_SUB     6 // output = a1 - a2 - v3
#define UDI_GFX_OPERATOR_MUL     7 // output = a1 * a2
#define UDI_GFX_OPERATOR_DIV     8 // output = a1 / a2
#define UDI_GFX_OPERATOR_MAD     9 // output = a1 * a2 + a3
#define UDI_GFX_OPERATOR_FRC    10 // output = (a1 * a2) / a3
#define UDI_GFX_OPERATOR_SHR    11 // output = a1 >> (a2 + v3)
#define UDI_GFX_OPERATOR_SHL    12 // output = a1 << (a2 + v3)
#define UDI_GFX_OPERATOR_ROR    13 // output = a1 >> a2 (over a3 bits)
#define UDI_GFX_OPERATOR_ROL    14 // output = a1 << a2 (over a3 bits)
#define UDI_GFX_OPERATOR_SAR    15 // output = a1 >> a2 (width is a3 bits, i.e. empties are filled with bit a3-1)
#define UDI_GFX_OPERATOR_SAL    16 // output = a1 <<< (a2 + v3) (empties filled with bit 0)
#define UDI_GFX_OPERATOR_AND    17 // output = a1 & a2
#define UDI_GFX_OPERATOR_OR     18 // output = a1 | a2 | v3
#define UDI_GFX_OPERATOR_NOT    19 // output = ~a1
#define UDI_GFX_OPERATOR_XOR    20 // output = a1 ^ a2 ^ v3
#define UDI_GFX_OPERATOR_NEG    21 // output = -a1
#define UDI_GFX_OPERATOR_SEG    22 // output = (a1 >> v2) & (2**v3-1) (select v3 bits starting from bit v2)
#define UDI_GFX_OPERATOR_RANGE  23 // output = (a1 > a2) ? a2 : ((a1 < a3) ? a3 : a1)
#define UDI_GFX_OPERATOR_CONST  24 // output = v1
#define UDI_GFX_OPERATOR_ATTR   25 // output = property[a1 + v2]
#define UDI_GFX_OPERATOR_SWITCH 26 // output = output[(a1 % v3) + v2]
#define UDI_GFX_OPERATOR_BUFFER 27 // output = buffer[a1][a2] (buffer is v3 bits per entry)
#define UDI_GFX_OPERATOR_X      28 // output = output x pixel
#define UDI_GFX_OPERATOR_Y      29 // output = output y pixel
#define UDI_GFX_OPERATOR_TX     30 // output = horizontal tile index belonging to output pixel
#define UDI_GFX_OPERATOR_TY     31 // output = vertical tile index belonging to output pixel
#define UDI_GFX_OPERATOR_TXOFF  32 // output = horizontal offset from start of tile
#define UDI_GFX_OPERATOR_TYOFF  33 // output = vertical offset from start of tile
#define UDI_GFX_OPERATOR_INPUT  34 // output = input engine[x][y]   component v1
#define UDI_GFX_OPERATOR_DINPUT 35 // output = input engine[a1][a2] component v3
 
 
 
// Constant: UDI_GFX_PROVIDER_OPS_NUM
// the ops number used for the graphics driver
#define UDI_GFX_PROVIDER_OPS_NUM 1
 
// Constant: UDI_GFX_CLIENT_OPS_NUM
// the ops number used for the graphics application
#define UDI_GFX_CLIENT_OPS_NUM 2
 
// Structure: udi_gfx_bind_cb_t
// Contains the operations of a driver binding request
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
} udi_gfx_bind_cb_t;
#define UDI_GFX_BIND_CB_NUM 1
 
// Function: udi_block_bind_req
// function pointer prototype for connecting to a block device
// 
// in:
//     cb - A pointer to a <udi_block_bind_cb_t>
//
typedef void udi_gfx_bind_req_op_t (udi_gfx_bind_cb_t *cb );
udi_gfx_bind_req_op_t udi_gfx_bind_req;
 
// Function: udi_gfx_bind_ack
// function pointer prototype for acknowledging a connection request
// 
// in:
//     cb      - A pointer to a <udi_gfx_bind_cb_t>
//     sockets - The number of addressable socket components
//     engines - The number of addressable engine components
//     status  - The result of the bind operation
//
typedef void udi_gfx_bind_ack_op_t (udi_gfx_bind_cb_t *cb, udi_index_t sockets, udi_index_t engines, udi_status_t status );
udi_gfx_bind_ack_op_t udi_gfx_bind_ack;
 
// Function: udi_gfx_unbind_req
// function pointer prototype for disconnecting a block device
// 
// in:
//     cb - A pointer to a <udi_block_bind_cb_t>
//
typedef void udi_gfx_unbind_req_op_t (udi_gfx_bind_cb_t *cb );
udi_gfx_unbind_req_op_t udi_gfx_unbind_req;
 
// Function: udi_gfx_unbind_ack
// function pointer prototype for connecting to a block device
// 
// in:
//     cb - A pointer to a <udi_gfx_bind_cb_t>
//
typedef void udi_gfx_unbind_ack_op_t (udi_gfx_bind_cb_t *cb );
udi_gfx_unbind_ack_op_t udi_gfx_unbind_ack;
 
// Structure: udi_gfx_state_cb_t
// Contains the operations of a read/write transaction
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_ubit32_t subsystem;
    udi_ubit32_t attribute;
} udi_gfx_state_cb_t;
#define UDI_GFX_STATE_CB_NUM 2
 
// Function: udi_gfx_set_engine_req
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_set_engine_req_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t value);
udi_gfx_set_engine_req_op_t udi_gfx_set_engine_req;
 
// Function: udi_gfx_set_connector_req
// function pointer prototype for setting an connector state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_set_connector_req_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t value);
udi_gfx_set_connector_req_op_t udi_gfx_set_connector_req;
 
// Function: udi_gfx_set_engine_ack
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_set_engine_ack_op_t (udi_gfx_state_cb_t *cb );
udi_gfx_set_engine_ack_op_t udi_gfx_set_engine_ack;
 
// Function: udi_gfx_set_connector_ack
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_set_connector_ack_op_t (udi_gfx_state_cb_t *cb );
udi_gfx_set_connector_ack_op_t udi_gfx_set_connector_ack;
 
// Function: udi_gfx_get_engine_req
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_get_engine_req_op_t (udi_gfx_state_cb_t *cb );
udi_gfx_get_engine_req_op_t udi_gfx_get_engine_req;
 
// Function: udi_gfx_get_connector_req
// function pointer prototype for setting an connector state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_get_connector_req_op_t (udi_gfx_state_cb_t *cb );
udi_gfx_get_connector_req_op_t udi_gfx_get_connector_req;
 
// Function: udi_gfx_get_engine_ack
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_get_engine_ack_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t value);
udi_gfx_get_engine_ack_op_t udi_gfx_get_engine_ack;
 
// Function: udi_gfx_get_connector_ack
// function pointer prototype for setting an engine state
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_get_connector_ack_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t value);
udi_gfx_get_connector_ack_op_t udi_gfx_get_connector_ack;
 
 
// Structure: udi_gfx_range_cb_t
// Contains the operations of a range request transaction
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_ubit32_t subsystem;
    udi_ubit32_t attribute;
    udi_buf_t * rangedata;  
} udi_gfx_range_cb_t;
#define UDI_GFX_RANGE_CB_NUM 3
 
// Function: udi_gfx_range_engine_req
// function pointer prototype for getting an engine property range
// 
// in:
//     cb - A pointer to a <udi_gfx_range_cb_t>
//
typedef void udi_gfx_range_engine_req_op_t (udi_gfx_range_cb_t *cb );
udi_gfx_range_engine_req_op_t udi_gfx_range_engine_req;
 
// Function: udi_gfx_range_connector_req
// function pointer prototype for getting a connector property range
// 
// in:
//     cb - A pointer to a <udi_gfx_range_cb_t>
//
typedef void udi_gfx_range_connector_req_op_t (udi_gfx_range_cb_t *cb );
udi_gfx_range_connector_req_op_t udi_gfx_range_connector_req;
 
// Function: udi_gfx_range_engine_ack
// function pointer prototype for replying an engine property range
// 
// in:
//     cb - A pointer to a <udi_gfx_range_cb_t>
//
typedef void udi_gfx_range_engine_ack_op_t (udi_gfx_range_cb_t *cb );
udi_gfx_range_engine_ack_op_t udi_gfx_range_engine_ack;
 
// Function: udi_gfx_range_connector_ack
// function pointer prototype for replying a connector property range
// 
// in:
//     cb - A pointer to a <udi_gfx_range_cb_t>
//
typedef void udi_gfx_range_connector_ack_op_t (udi_gfx_range_cb_t *cb );
udi_gfx_range_connector_ack_op_t udi_gfx_range_connector_ack;
 
 
// Structure: udi_gfx_command_cb_t
// Contains the operations of a command sequence
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_buf_t * commanddata;
} udi_gfx_command_cb_t;
#define UDI_GFX_COMMAND_CB_NUM 4
 
// Function: udi_gfx_command
// function pointer prototype for sending command data
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_command_req_op_t (udi_gfx_command_cb_t *cb );
udi_gfx_command_req_op_t udi_gfx_command_req;
 
// Function: udi_gfx_command_ack
// function pointer prototype for sending command data
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_command_ack_op_t (udi_gfx_command_cb_t *cb);
udi_gfx_command_ack_op_t udi_gfx_command_ack;
 
/* Structure: udi_gfx_provider_ops_t
 The graphics metalanguage e*ntry points (provider side)
 */
typedef const struct {
    udi_channel_event_ind_op_t          *channel_event_ind_op;
    udi_gfx_bind_req_op_t               *gfx_bind_req_op;
    udi_gfx_unbind_req_op_t             *gfx_unbind_req_op;
    udi_gfx_set_connector_req_op_t      *gfx_set_connector_req_op;
    udi_gfx_set_engine_req_op_t         *gfx_set_engine_req_op;
    udi_gfx_get_connector_req_op_t      *gfx_get_connector_req_op;
    udi_gfx_get_engine_req_op_t         *gfx_get_engine_req_op;
    udi_gfx_range_connector_req_op_t    *gfx_range_connector_req_op;
    udi_gfx_range_engine_req_op_t       *gfx_range_engine_req_op;
    udi_gfx_command_req_op_t            *gfx_command_op;
} udi_gfx_provider_ops_t;
 
/* Structure: udi_gfx_client_ops_t
 * The graphics metalanguage entry points (client side)
 */
typedef const struct {
    udi_channel_event_ind_op_t          *channel_event_ind_op;
    udi_gfx_bind_ack_op_t               *udi_gfx_bind_ack;
    udi_gfx_unbind_ack_op_t             *udi_gfx_unbind_ack;
    udi_gfx_set_connector_ack_op_t      *udi_gfx_set_connector_ack;
    udi_gfx_set_engine_ack_op_t         *udi_gfx_set_engine_ack;
    udi_gfx_get_connector_ack_op_t      *udi_gfx_get_connector_ack;
    udi_gfx_get_engine_ack_op_t         *udi_gfx_get_engine_ack;
    udi_gfx_range_connector_ack_op_t    *udi_gfx_range_connector_ack;
    udi_gfx_range_engine_ack_op_t       *udi_gfx_range_engine_ack;
    udi_gfx_command_ack_op_t            *udi_gfx_command_ack;
} udi_gfx_client_ops_t;
 
 
// temporary
#ifndef UDI_ANNOY_ME
void EngineReturnSimpleRange (int source, int index, int prop, int first, int last, int modulus);
void ConnectorReturnSimpleRange (int source, int index, int prop, int first, int last, int modulus);
void EngineReturnConstantRange (int source, int index, int prop, int value);
void ConnectorReturnConstantRange (int source, int index, int prop, int value);
void EngineReturnBooleanRange (int source, int index, int prop, int value1, int value2);
void ConnectorReturnBooleanRange (int source, int index, int prop, int value1, int value2);
#endif
 
#endif

