/*
 * Acess2 VGA Driver
 * - By John Hodge (thePowersGang)
 */

#define VGA_CRTC_IDX	0x3B4
#define VGA_CRTC_DATA	0x3B5
#define VGA_FEAT_IS1	0x3BA	// Features / Input Status 1 (B/W Chips)
#define VGA_ATTR_WRITE	0x3C0
#define VGA_ATTR_READ	0x3C1
#define VGA_MISC_IS0	0x3C2	// Misc Output / Input Status 0
#define VGA_MBSLEEP	0x3C3	// "Motherboard Sleep"
#define VGA_SEQ_IDX	0x3C4
#define VGA_SEQ_DATA	0x3C5
#define VGA_DACMASK	0x3C6
#define VGA_PADR_DACST	0x3C7
#define VGA_PIXWRMODE	0x3C8
#define VGA_PIXDATA	0x3C9
#define VGA_FEATRD	0x3CA
//                	0x3CB
#define VGA_MISCOUT	0x3CC
//                	0x3CD
#define VGA_GRAPH_IDX	0x3CE
#define VGA_GRAPH_DATA	0x3CF
//                   	0x3D0 -- 0x3D3
#define VGA_CRTC_IDX	0x3D4
#define VGA_CRTC_DATA	0x3D5
//                   	0x3D6 -- 0x3D9
#define VGA_FEAT_IS1_C	0x3DA	// Features / Input Status 1 (Colour Chips)

// === CODE ===
void VGA_WriteAttr(Uint8 Index, Uint8 Data)
{
	Index &= 0x1F;
	SHORTLOCK(&glVGA_Attr);
	inb(0x3DA);
	outb(VGA_ATTR_WRITE, Index);
	outb(VGA_ATTR_WRITE, Data);
	SHORTREL(&glVGA_Attr);
}

Uint8 VGA_ReadAttr(Uint8 Index)
{
	Uint8	ret;
	SHORTLOCK(&glVGA_Attr);
	inb(0x3DA);
	outb(VGA_ATTR_WRITE, Index);
	ret = inb(VGA_ATTR_READ);
	SHORTREL(&glVGA_Attr);
	return ret;
}

void VGA_WriteSeq(Uint8 Index, Uint8 Data)
{
	outb(VGA_SEQ_IDX, Index);
	outb(VGA_SEQ_DATA, Data);
}
Uint8 VGA_ReadSeq(Uint8 Index)
{
	outb(VGA_SEQ_IDX, Index);
	return inb(VGA_SEQ_DATA);
}
void VGA_WriteGraph(Uint8 Index, Uint8 Data)
{
	outb(VGA_GRAPH_IDX, Index);
	outb(VGA_GRAPH_DATA, Data);
}
Uint8 VGA_ReadGraph(Uint8 Index)
{
	outb(VGA_GRAPH_IDX, Index);
	return inb(VGA_GRAPH_DATA);
}

void VGA_WriteMiscOut(Uint8 Data)
{
	outb(VGA_MISC_IS0, Data);
}
Uint8 VGA_ReadMiscOut(void)
{
	return inb(VGA_MISCOUT);
}
