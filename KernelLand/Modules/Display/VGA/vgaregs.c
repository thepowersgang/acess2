/*
 * Acess2 VGA Driver
 * - By John Hodge (thePowersGang)
 */


// === CODE ===
void VGA_WriteAttr(Uint8 Index, Uint8 Data)
{
	Index &= 0x1F;
	SHORTLOCK(&glVGA_Attr);
	inb(0x3DA);
	outb(0x3C0, Index);
	outb(0x3C0, Data);
	SHORTREL(&glVGA_Attr);
}

Uint8 VGA_ReadAttr(Uint8 Index)
{
	Uint8	ret;
	SHORTLOCK(&glVGA_Attr);
	inb(0x3DA);
	outb(0x3C0, Index);
	ret = inb(0x3C1);
	SHORTREL(&glVGA_Attr);
	return ret;
}

void VGA_WriteSeq(Uint8 Index, Uint8 Data)
{
	outb(0x3C4, Index);
	outb(0x3C5, Data);
}
Uint8 VGA_ReadSeq(Uint8 Index)
{
	outb(0x3C4, Index);
	return inb(0x3C5);
}
void VGA_WriteGraph(Uint8 Index, Uint8 Data)
{
	outb(0x3CE, Index);
	outb(0x3CF, Data);
}
Uint8 VGA_ReadGraph(Uint8 Index)
{
	outb(0x3CE, Index);
	return inb(0x3CF);
}

void VGA_WriteMiscOut(Uint8 Data)
{
	outb(0x3C2, Data);
}
Uint8 VGA_ReadMiscOut(void)
{
	return inb(0x3CC);
}
