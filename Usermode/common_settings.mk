

CPPFLAGS += $(addprefix -I,$(wildcard $(ACESSUSERDIR)Libraries/*/include_exp/))
CPPFLAGS += -I$(ACESSUSERDIR)/include/ -DARCHDIR_is_$(ARCHDIR)
CPPFLAGS += -I $(ACESSDIR)/Externals/Output/$(ARCHDIR)/include
CFLAGS += -std=gnu99 -g
LDFLAGS += -L $(ACESSDIR)/Externals/Output/$(ARCHDIR)/lib

CRTI := $(OUTPUTDIR)Libs/crti.o
CRTBEGIN := $(shell $(CC) $(CFLAGS) -print-file-name=crtbegin.o 2>/dev/null)
CRTBEGINS := $(shell $(CC) $(CFLAGS) -print-file-name=crtbeginS.o 2>/dev/null)
CRT0 := $(OUTPUTDIR)Libs/crt0.o
CRT0S := $(OUTPUTDIR)Libs/crt0S.o
CRTEND := $(shell $(CC) $(CFLAGS) -print-file-name=crtend.o 2>/dev/null)
CRTENDS := $(shell $(CC) $(CFLAGS) -print-file-name=crtendS.o 2>/dev/null)
CRTN := $(OUTPUTDIR)Libs/crtn.o
LIBGCC_PATH = $(shell $(CC) -print-libgcc-file-name 2>/dev/null)
