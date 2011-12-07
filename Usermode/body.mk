
OBJ := $(call fcn_src2obj,$(SRCS))
ALL_OBJ := $(ALL_OBJ) $(OBJ)
OBJ-$(DIR) := $(OBJ)
BIN-$(DIR) := $(BIN)

# Rules
.PHONY: all-$(DIR) clean-$(DIR)
all-$(DIR): $(BIN)
clean-$(DIR): clean-%: 
	$(eval BIN=$(BIN-$*/))
	$(eval OBJ=$(OBJ-$*/))
	$(RM) $(BIN) $(OBJ)

