#
#

CPPFLAGS = -I./include

# - UDI Library Files
LIB_OBJS := logging.o strmem.o imc.o mem.o buf.o cb.o
LIB_OBJS += queues.o time.o
LIB_OBJS += meta_mgmt.o meta_gio.o
LIB_OBJS += physio.o physio/meta_bus.o physio/meta_intr.o physio/pio.o physio/dma.o
LIB_OBJS += scsi.o
# - UDI->Acess Translation Layer
TRANS_OBJS := bus_pci.o

OBJ  = main.o channels.o
OBJ += $(LIB_OBJS:%=udi_lib/%) $(TRANS_OBJS:%=trans/%)
NAME = UDI

-include ../Makefile.tpl
