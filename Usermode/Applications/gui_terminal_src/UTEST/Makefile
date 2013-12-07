#
#
#

CPPFLAGS := -I ../include -I ../../../Libraries/ld-acess.so_src/include_exp
CFLAGS := -Wall -std=c99
CFLAGS += $(CPPFLAGS)
TESTFILES := vt100

OBJDIR := stage/obj/
BINDIR := stage/

.PHONY: all clean
# run $(TESTFILES:%=run_%)

all: $(TESTFILES:%=$(BINDIR)TEST_%)

#run: all $(TESTFILES:%=run_%)

.PRECIOUS:

#run_%: $(BINDIR)TEST_%
#	./$(BINDIR)TEST_%

$(BINDIR)TEST_%: $(OBJDIR)TEST_%.o $(OBJDIR)__%.o $(OBJDIR)test_common.o
	@mkdir -p $(dir $@)
	@echo [CC] -o $@
	@$(CC) -o $@ $(OBJDIR)TEST_$*.o $(OBJDIR)__$*.o $(OBJDIR)test_common.o $(LDFLAGS)

$(OBJDIR)%.o: %.c Makefile
	@mkdir -p $(dir $@)
	@echo [CC] -c $< -o $@
	@$(CC) -c $< -o $@ $(CFLAGS)
$(OBJDIR)__%.o: ../%.c Makefile
	@mkdir -p $(dir $@)
	@echo [CC] -c $< -o $@
	@$(CC) -c $< -o $@ $(CFLAGS)


