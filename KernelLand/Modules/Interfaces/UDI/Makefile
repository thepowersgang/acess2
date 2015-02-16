#
#

CPPFLAGS = -I../../../../UDI/include -Iinclude
CPPFLAGS += -D UDI_VERSION=0x101 -D UDI_PHYSIO_VERSION=0x101 -D UDI_NIC_VERSION=0x101

# - UDI Library Files
LIB_OBJS := core/logging.o core/strmem.o core/imc.o core/mem.o core/buf.o
LIB_OBJS += core/queues.o core/time.o core/attr.o core/mei.o core/cb.o
LIB_OBJS += core/layout.o
LIB_OBJS += core/meta_mgmt.o core/meta_gio.o
LIB_OBJS += physio.o physio/meta_bus.o physio/pio.o physio/dma.o
LIB_OBJS += scsi.o
LIB_OBJS += udi_nic.o
# - UDI->Acess Translation Layer
TRANS_OBJS := bus_pci.o nsr.o gio_uart.o

OBJ  = main.o channels.o deferred_calls.o management_agent.o
OBJ += $(LIB_OBJS:%=udi_lib/%) $(TRANS_OBJS:%=trans/%)
NAME = UDI

-include ../Makefile.tpl

ifeq ($(ARCHDIR),x86)
 CPPFLAGS += -DUDI_ABI_is_ia32
else
 $(error TODO: Support other UDI ABIs [$(ARCHDIR) unsupported])
endif
