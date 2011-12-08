
OBJ := $(call fcn_src2obj,$(SRCS))

$(call fcn_addbin,$(BIN),$(OBJ))

# Rules
.PHONY: all-$(DIR) clean-$(DIR)
all-$(DIR): $(BIN)
clean-$(DIR): clean-%: 
	$(eval BIN=$(BIN-$*/))
	$(eval OBJ=$(OBJ-$*/))
	$(RM) $(BIN) $(OBJ) $(OBJ:%=%.dep)

