

CPPFLAGS += $(addprefix -I,$(wildcard $(ACESSUSERDIR)Libraries/*/include_exp/))
CPPFLAGS += -I$(ACESSUSERDIR)/include/ -DARCHDIR_is_$(ARCHDIR)
CPPFLAGS += -I $(ACESSDIR)/Externals/Output/$(ARCHDIR)/include
CFLAGS += -std=gnu99 -g
LDFLAGS += -L $(ACESSDIR)/Externals/Output/$(ARCHDIR)/lib
