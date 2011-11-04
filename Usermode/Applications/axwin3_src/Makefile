
NAME = AxWin3
DIRS = WM libaxwin3.so_src Interface

SUBMAKE = $(MAKE) --no-print-directory

all:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true
install:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true

clean:
	@$(foreach DIR,$(DIRS), $(SUBMAKE) -C $(DIR) $@ &&) true
