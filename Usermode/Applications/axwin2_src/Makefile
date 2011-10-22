
NAME = AxWin2
DIRS = WM Shell_src

SUBMAKE = $(MAKE) --no-print-directory

all:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true
install:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true

clean:
	@$(foreach DIR,$(DIRS), $(SUBMAKE) -C $(DIR) $@ &&) true
