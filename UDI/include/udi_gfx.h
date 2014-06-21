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
 * Origin:
 *     http://www.d-rift.nl/combuster/mos3/?p=viewsource&file=/include/common/udi_gfx.h
 */

/* note that the specification, and thus, the contents of this file is not fixed. */

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

/* Constant: UDI_GFX_PROP_ENABLE
 *
 * Valid values:
 *     0 - disabled
 *     1 - enabled
 *     2 - reset
 *
 * Ranges:
 *     Must include at least 1
 *
 * The primary state of the connector or engine. An enabled state indicates
 * it is functioning and generating live output. A disabled state is one where
 * it is not contributing to any output but is otherwise functional. Finally
 * the reset state is where the driver is free to deallocate all resources 
 * corresponding to this component and trash any state not referenced by other
 * components.
 *
 * A disabled or reset engine forwards all data from the previous stage 
 * unmodified. The disabled state indicates that the component might be 
 * returned to its enabled state within short notice.
 *
 * A disabled connector will not send pixel data, but can perform other 
 * initialisation communication such as DDC. A reset connector will not respond
 * in any fashion and can not be used for other purposes. Hardware is expected
 * to be powered down in such state.
 *
 * Users should expect significant delays when moving components in and out of
 * the reset state. Moving engines between the enabled and disabled state should
 * take effect within one frame, such transition should take effect on a frame 
 * boundary when supported.
 */
#define UDI_GFX_PROP_ENABLE 0

#define UDI_GFX_PROP_ENABLE_DISABLED 0
#define UDI_GFX_PROP_ENABLE_ENABLED 1
#define UDI_GFX_PROP_ENABLE_RESET 2
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

/* engine properties */

/* Constant: UDI_GFX_PROP_CLIP
 *
 * Valid values:
 *     0 - points outside width x height are passed unmodified from input
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

/* Constant: UDI_GFX_PROP_TRANSLATEX
 * 
 * Valid values:
 *     Any, signed value.
 *
 * Ranges:
 *     Any non-empty set. Typical values are hardwired zero, continuous
 *     between -WIDTH and WIDTH, -WIDTH to zero inclusive, or all possible values
 *
 * The horizontal offset where drawing starts. A positive value means the top-left 
 * corner moves towards the right end of the screen, a negative value moves the
 * origin off the screen on the left side. Clipped areas moved off the screen do 
 * not reappear on the opposite side.
 *
 * With clipping enabled, this field combined with <UDI_GFX_PROP_WIDTH> 
 * determines the area where the image should be drawn, which is the horizontal 
 * range from UDI_GFX_PROP_TRANSLATEX to UDI_GFX_PROP_WIDTH + 
 * UDI_GFX_PROP_TRANSLATEX - 1
 */
#define UDI_GFX_PROP_TRANSLATEX 7

/* Constant: UDI_GFX_PROP_TRANSLATEY
 *
 * Valid values:
 *     Any signed value.
 *
 * Ranges:
 *     Any non-empty set. Typical values are hardwired zero, continuous
 *     between -WIDTH and WIDTH, or all possible values
 *
 * See <UDI_GFX_PROP_TRANSLATEX> but for the Y direction.
 */
#define UDI_GFX_PROP_TRANSLATEY 8

#define UDI_GFX_PROP_GL_VERSION 14
#define UDI_GFX_PROP_GLES_VERSION 15
#define UDI_GFX_PROP_STATE_BLOCK 16
#define UDI_GFX_PROP_COLOR_BITS 22
#define UDI_GFX_PROP_GL_TARGET 23

/* Constant: UDI_GFX_PROP_STOCK_FORMAT
 *
 * Value:
 *     Zero, or any constant from <UDI_GFX_STOCK_FORMAT>
 *
 * Ranges:
 *     Any non-empty set of valid values.
 *
 * This field indicates the storage format is one from a limited set of 
 * typical configurations. If the field is zero, the engine is not knowingly
 * configured as a common framebuffer. If nonzero, the operator chain and any
 * dependent settings are defined to be functionally equivalent to that of a
 * typical framebuffer device.
 *
 * The value zero does not imply that the device does not actually follow a
 * set convention. This saves drivers from writing elaborate checking code
 * to determine the condition in question.
 *
 * Writing this field potentially modifies other property fields within the
 * same engine to establish the requested configuration. Manually writing such 
 * properties after changing this setting may in turn revert this property to
 * the zero state, even if there was no modification or the behaviour is still
 * as previously advertised by this property.
 */
#define UDI_GFX_PROP_STOCK_FORMAT 27

/* Constant: UDI_GFX_PROP_OPERATOR_COUNT
 * 
 * Valid values:
 *     Any non-zero positive number
 * 
 * Ranges:
 *     Most likely constant. Can be any set of valid values.
 *
 * The current value is the number of entries in the operator tree that can
 * be requested for this engine using <udi_gfx_get_engine_operator_req> and
 * <udi_gfx_get_engine_operator_ack>
 */
#define UDI_GFX_PROP_OPERATOR_COUNT 28

#if 0
/* properties for removal: */
#define UDI_GFX_PROP_STORE_COUNT 24       // not generic
#define UDI_GFX_PROP_STORE_WIDTH 9        // not generic
#define UDI_GFX_PROP_STORE_HEIGHT 10      // not generic
#define UDI_GFX_PROP_STORE_BITS 11        // not generic
#define UDI_GFX_PROP_PALETTE 1024         // optional, can be derived from the operator tree
#define UDI_GFX_PROP_BUFFER 1025          // optional, can be derived from the operator tree
#define UDI_GFX_PROP_TILESHEET 1026       // optional, can be derived from the operator tree
#define UDI_GFX_PROP_OPERATOR_INDEX 17    // deprecated for dedicated methods
#define UDI_GFX_PROP_OPERATOR_OPCODE 18   // deprecated for dedicated methods
#define UDI_GFX_PROP_OPERATOR_ARG_1 19    // deprecated for dedicated methods
#define UDI_GFX_PROP_OPERATOR_ARG_2 20    // deprecated for dedicated methods
#define UDI_GFX_PROP_OPERATOR_ARG_3 21    // deprecated for dedicated methods
#define UDI_GFX_PROP_SOURCE_WIDTH 12      // should have been documented when I still knew what it did.
#define UDI_GFX_PROP_SOURCE_HEIGHT 13     // should have been documented when I still knew what it did.
#define UDI_GFX_PROP_INPUTX 25            // should have been documented when I still knew what it did.
#define UDI_GFX_PROP_INPUTY 26            // should have been documented when I still knew what it did.
#endif

/* connector properties */
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
#define UDI_GFX_CONNECTOR_MEMBUFFER 9

/**
 * Enumeration: UDI_GFX_OPERATOR
 * Lists the display output operator
 * 
 * a1/a2/a3 represents taking the output of a previous operation
 * v1/v2/v3 represents the literal value of that argument
 */
#define UDI_GFX_OPERATOR_RGB     0 /* output = (color) red(a1) + green(a2) + blue(a3) (each component is UDI_GFX_PROP_COLOR_BITS*/
#define UDI_GFX_OPERATOR_YUV     1 /* output = (color) Y(a1) + U(a2) + V(a3)*/
#define UDI_GFX_OPERATOR_YIQ     2 /* output = (color) Y(a1) + I(a2) + Q(a3)*/
#define UDI_GFX_OPERATOR_I       3 /* output = (color) intensity(a1)*/
#define UDI_GFX_OPERATOR_ALPHA   4 /* output = (color) a1 + alpha(a2)*/
#define UDI_GFX_OPERATOR_ADD     5 /* output = a1 + a2 + v3*/
#define UDI_GFX_OPERATOR_SUB     6 /* output = a1 - a2 - v3*/
#define UDI_GFX_OPERATOR_MUL     7 /* output = a1 * a2*/
#define UDI_GFX_OPERATOR_DIV     8 /* output = a1 / a2*/
#define UDI_GFX_OPERATOR_MAD     9 /* output = a1 * a2 + a3*/
#define UDI_GFX_OPERATOR_FRC    10 /* output = (a1 * a2) / a3*/
#define UDI_GFX_OPERATOR_SHR    11 /* output = a1 >> (a2 + v3)*/
#define UDI_GFX_OPERATOR_SHL    12 /* output = a1 << (a2 + v3)*/
#define UDI_GFX_OPERATOR_ROR    13 /* output = a1 >> a2 (over a3 bits)*/
#define UDI_GFX_OPERATOR_ROL    14 /* output = a1 << a2 (over a3 bits)*/
#define UDI_GFX_OPERATOR_SAR    15 /* output = a1 >> a2 (width is a3 bits, i.e. empties are filled with bit a3-1)*/
#define UDI_GFX_OPERATOR_SAL    16 /* output = a1 <<< (a2 + v3) (empties filled with bit 0)*/
#define UDI_GFX_OPERATOR_AND    17 /* output = a1 & a2*/
#define UDI_GFX_OPERATOR_OR     18 /* output = a1 | a2 | v3*/
#define UDI_GFX_OPERATOR_NOT    19 /* output = ~a1*/
#define UDI_GFX_OPERATOR_XOR    20 /* output = a1 ^ a2 ^ v3*/
#define UDI_GFX_OPERATOR_NEG    21 /* output = -a1*/
#define UDI_GFX_OPERATOR_SEG    22 /* output = (a1 >> v2) & (2**v3-1) (select v3 bits starting from bit v2)*/
#define UDI_GFX_OPERATOR_RANGE  23 /* output = (a1 > a2) ? a2 : ((a1 < a3) ? a3 : a1)*/
#define UDI_GFX_OPERATOR_CONST  24 /* output = v1*/
#define UDI_GFX_OPERATOR_ATTR   25 /* output = property[a1 + v2]*/
#define UDI_GFX_OPERATOR_SWITCH 26 /* output = output[(a1 % v3) + v2]*/
#define UDI_GFX_OPERATOR_BUFFER 27 /* output = buffer[a1][a2] (buffer is v3 bits per entry)*/
#define UDI_GFX_OPERATOR_X      28 /* output = output x pixel*/
#define UDI_GFX_OPERATOR_Y      29 /* output = output y pixel*/
#define UDI_GFX_OPERATOR_TX     30 /* output = horizontal tile index belonging to output pixel*/
#define UDI_GFX_OPERATOR_TY     31 /* output = vertical tile index belonging to output pixel*/
#define UDI_GFX_OPERATOR_TXOFF  32 /* output = horizontal offset from start of tile*/
#define UDI_GFX_OPERATOR_TYOFF  33 /* output = vertical offset from start of tile*/
#define UDI_GFX_OPERATOR_INPUT  34 /* output = input engine[x][y]   component v1*/
#define UDI_GFX_OPERATOR_DINPUT 35 /* output = input engine[a1][a2] component v3*/

/* Enumeration: UDI_GFX_STOCK_FORMAT
 * Lists stock configurations
 *
 * When a stock configuration is used, the device is set to behave as a 
 * simple framebuffer device. The <UDI_GFX_PROP_WIDTH> and <UDI_GFX_PROP_HEIGHT>
 * determine the virtual size of the framebuffer, and <UDI_GFX_PROP_TRANSLATEX>
 * and <UDI_GFX_PROP_TRANSLATEY> indicate the offset into that framebuffer 
 * that is visible (which are typically restricted to negative values)
 */
#define UDI_GFX_STOCK_FORMAT_UNKNOWN  0
#define UDI_GFX_STOCK_FORMAT_R8G8B8X8 1
#define UDI_GFX_STOCK_FORMAT_B8G8R8X8 2
#define UDI_GFX_STOCK_FORMAT_R8G8B8   3
#define UDI_GFX_STOCK_FORMAT_B8G8R8   4
#define UDI_GFX_STOCK_FORMAT_R5G6B5   5
#define UDI_GFX_STOCK_FORMAT_B5G6R5   6
#define UDI_GFX_STOCK_FORMAT_R5G5B5X1 7
#define UDI_GFX_STOCK_FORMAT_B5G5R5X1 8
#define UDI_GFX_STOCK_FORMAT_N8 9

/*
 * Enumeration: UDI_GFX_BUFFER_INFO_FLAG
 * Lists behavioural patterns for direct buffer accesses.
 */
#define UDI_GFX_BUFFER_INFO_FLAG_R              0x0001  /* buffer can be read*/
#define UDI_GFX_BUFFER_INFO_FLAG_W              0x0002  /* buffer can be written*/
#define UDI_GFX_BUFFER_INFO_FLAG_BITALIGN_ENTRY 0x0004  /* for non-multiple-of-eight buffer slot sizes, align on byte boundary every unit*/
#define UDI_GFX_BUFFER_INFO_FLAG_BITALIGN_ROW   0x0008  /* for non-multiple-of-eight buffer slot sizes, align only the start of the row*/


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

// Function: udi_gfx_set_engine_nak
// function pointer prototype for setting an engine state
// 
// in:
//     cb     - A pointer to a <udi_gfx_state_cb_t>
//     status - An UDI status value indicative of the error
//
typedef void udi_gfx_set_engine_nak_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t status);
udi_gfx_set_engine_nak_op_t udi_gfx_set_engine_nak;

// Function: udi_gfx_set_connector_nak
// function pointer prototype for setting an engine state
// 
// in:
//     cb     - A pointer to a <udi_gfx_state_cb_t>
//     status - An UDI status value indicative of the error
//
typedef void udi_gfx_set_connector_nak_op_t (udi_gfx_state_cb_t *cb, udi_ubit32_t status);
udi_gfx_set_connector_nak_op_t udi_gfx_get_connector_nak;

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

// Function: udi_gfx_get_engine_operator_req
// function pointer prototype for requesting the engine operator layout
// 
// in:
//     cb - A pointer to a <udi_gfx_state_cb_t>
//
typedef void udi_gfx_get_engine_operator_req_op_t (udi_gfx_range_cb_t *cb );
udi_gfx_get_engine_operator_req_op_t udi_gfx_get_engine_operator_req;

// Function: udi_gfx_get_engine_operator_ack
// function pointer prototype for replying the engine operator layout
// 
// in:
//     cb   - A pointer to a <udi_gfx_state_cb_t>
//     op   - The operator performed at this index
//     arg1 - the first argument to this operator
//     arg2 - the second argument to this operator
//     arg3 - the third argument to this operator
//
typedef void udi_gfx_get_engine_operator_ack_op_t (udi_gfx_range_cb_t *cb, udi_ubit32_t op, udi_ubit32_t arg1, udi_ubit32_t arg2, udi_ubit32_t arg3 );
udi_gfx_get_engine_operator_ack_op_t udi_gfx_get_engine_operator_ack;



// Structure: udi_gfx_command_cb_t
// Contains the operations of a command sequence
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_buf_t * commanddata;
} udi_gfx_command_cb_t;
#define UDI_GFX_COMMAND_CB_NUM 4

// Function: udi_gfx_connector_command_req
// function pointer prototype for sending command data to the output connector
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_connector_command_req_op_t (udi_gfx_command_cb_t *cb );
udi_gfx_connector_command_req_op_t udi_gfx_connector_command_req;

// Function: udi_gfx_engine_command_req
// function pointer prototype for sending command data to the engine
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_engine_command_req_op_t (udi_gfx_command_cb_t *cb );
udi_gfx_engine_command_req_op_t udi_gfx_engine_command_req;

// Function: udi_gfx_connector_command_ack
// function pointer prototype for sending command data replies
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_connector_command_ack_op_t (udi_gfx_command_cb_t *cb);
udi_gfx_connector_command_ack_op_t udi_gfx_connector_command_ack;

// Function: udi_gfx_engine_command_ack
// function pointer prototype for sending engine data replies
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_engine_command_ack_op_t (udi_gfx_command_cb_t *cb);
udi_gfx_engine_command_ack_op_t udi_gfx_engine_command_ack;

// Structure: udi_gfx_buffer_cb_t
// Contains a description of a buffer, or area thereof
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_ubit32_t buffer_index;
} udi_gfx_buffer_info_cb_t;

// Function: udi_gfx_buffer_info_req
// function pointer prototype for getting buffer configuration information
// 
// in:
//     cb - A pointer to a <udi_gfx_command_cb_t>
//
typedef void udi_gfx_buffer_info_req_op_t (udi_gfx_buffer_info_cb_t *cb);
udi_gfx_buffer_info_req_op_t udi_gfx_buffer_info_req;

// Function: udi_gfx_buffer_info_ack
// function pointer prototype for getting buffer configuration information
// 
// in:
//     cb       - A pointer to a <udi_gfx_command_cb_t>
//     width    - The width of the buffer
//     height   - The height of the buffer
//     bitsper  - The number of bits read from the buffer per pixel unit
//     flags    - A bitfield of <UDI_GFX_BUFFER_FLAGS> indicating the exposed 
//                capabilities of this buffer
//
// Note that bitsper might not be a multiple of eight.
//
typedef void udi_gfx_buffer_info_ack_op_t (udi_gfx_buffer_info_cb_t *cb, udi_ubit32_t width, udi_ubit32_t height, udi_ubit32_t bitsper, udi_ubit32_t flags);
udi_gfx_buffer_info_ack_op_t udi_gfx_buffer_info_ack;

// Structure: udi_gfx_buffer_cb_t
// Contains a description of a buffer, or area thereof
typedef struct {
    // Variable: gcb
    // The main control block
    udi_cb_t gcb;    
    udi_ubit32_t buffer_index;
    udi_ubit32_t x;
    udi_ubit32_t y;
    udi_ubit32_t width;
    udi_ubit32_t height;
    udi_buf_t * buffer;
} udi_gfx_buffer_cb_t;

// Function: udi_gfx_buffer_write_req_op_t
// function pointer prototype for writing raw hardware buffers
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_write_req_op_t (udi_gfx_buffer_cb_t *cb);
udi_gfx_buffer_write_req_op_t udi_gfx_buffer_write_req;

// Function: udi_gfx_buffer_write_req_op_t
// function pointer prototype for reading raw hardware buffers
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_read_req_op_t (udi_gfx_buffer_cb_t *cb);
udi_gfx_buffer_read_req_op_t udi_gfx_buffer_read_req;

// Function: udi_gfx_buffer_write_ack_op_t
// function pointer prototype for writing raw hardware buffers
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_write_ack_op_t (udi_gfx_buffer_cb_t *cb);
udi_gfx_buffer_write_ack_op_t udi_gfx_buffer_write_ack;

// Function: udi_gfx_buffer_write_ack_op_t
// function pointer prototype for reading raw hardware buffers
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_read_ack_op_t (udi_gfx_buffer_cb_t *cb);
udi_gfx_buffer_read_ack_op_t udi_gfx_buffer_read_ack;

// Function: udi_gfx_buffer_write_nak_op_t
// error handling for buffer writes
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_write_nak_op_t (udi_gfx_buffer_cb_t *cb, udi_ubit32_t status);
udi_gfx_buffer_write_nak_op_t udi_gfx_buffer_write_nak;

// Function: udi_gfx_buffer_write_nak_op_t
// error handling for buffer reads
// 
// in:
//     cb - A pointer to a <udi_gfx_buffer_cb_t>
//
typedef void udi_gfx_buffer_read_nak_op_t (udi_gfx_buffer_cb_t *cb, udi_ubit32_t status);
udi_gfx_buffer_read_nak_op_t udi_gfx_buffer_read_nak;

/* Structure: udi_gfx_provider_ops_t
 * 
 * The graphics metalanguage entry points (provider side)
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
    udi_gfx_get_engine_operator_req_op_t*gfx_get_engine_operator_req_op_t;
    udi_gfx_connector_command_req_op_t  *gfx_connector_command_op;
    udi_gfx_engine_command_req_op_t     *gfx_engine_command_op;
    udi_gfx_buffer_info_req_op_t        *gfx_buffer_info_req_op;
    udi_gfx_buffer_read_req_op_t        *gfx_buffer_read_req_op;
    udi_gfx_buffer_write_req_op_t       *gfx_buffer_write_req_op;
} udi_gfx_provider_ops_t;

/* Structure: udi_gfx_client_ops_t
 *
 * The graphics metalanguage entry points (client side)
 */
typedef const struct {
    udi_channel_event_ind_op_t          *channel_event_ind_op;
    udi_gfx_bind_ack_op_t               *udi_gfx_bind_ack;
    udi_gfx_unbind_ack_op_t             *udi_gfx_unbind_ack;
    udi_gfx_set_connector_ack_op_t      *udi_gfx_set_connector_ack;
    udi_gfx_set_engine_ack_op_t         *udi_gfx_set_engine_ack;
    udi_gfx_set_connector_nak_op_t      *udi_gfx_set_connector_nak;
    udi_gfx_set_engine_nak_op_t         *udi_gfx_set_engine_nak;
    udi_gfx_get_connector_ack_op_t      *udi_gfx_get_connector_ack;
    udi_gfx_get_engine_ack_op_t         *udi_gfx_get_engine_ack;
    udi_gfx_range_connector_ack_op_t    *udi_gfx_range_connector_ack;
    udi_gfx_range_engine_ack_op_t       *udi_gfx_range_engine_ack;
    udi_gfx_get_engine_operator_req_op_t*udi_gfx_get_engine_operator_ack;
    udi_gfx_connector_command_ack_op_t  *udi_gfx_connector_command_ack;
    udi_gfx_engine_command_ack_op_t     *udi_gfx_engine_command_ack;
    udi_gfx_buffer_info_ack_op_t        *gfx_buffer_info_ack;
    udi_gfx_buffer_read_ack_op_t        *gfx_buffer_read_ack;
    udi_gfx_buffer_write_ack_op_t       *gfx_buffer_write_ack;
    udi_gfx_buffer_read_nak_op_t        *gfx_buffer_read_nak;
    udi_gfx_buffer_write_nak_op_t       *gfx_buffer_write_nak;
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
