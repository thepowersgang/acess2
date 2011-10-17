
NAME = AxWin2
DIRS = WM Shell_src

SUBMAKE = $(MAKE) --no-print-directory

all:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR); $(SUBMAKE) -C $(DIR) $@ ;)
install:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR); $(SUBMAKE) -C $(DIR) $@ ;)

clean:
	@$(foreach DIR,$(DIRS), $(SUBMAKE) -C $(DIR) $@ ;)
