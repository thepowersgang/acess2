
-include ../Makefile.cfg

NAME = AxWin3
DIRS = WM libaxwin3.so_src Interface
FILES = $(addprefix -,AcessLogoSmall.sif)

SUBMAKE = $(MAKE) --no-print-directory

.PNONY: all install clean $(FILES)

all:
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true
install:
	@$(xMKDIR) $(DISTROOT)/Apps ; true
	@$(xMKDIR) $(DISTROOT)/Apps/AxWin ; true
	@$(xMKDIR) $(DISTROOT)/Apps/3.0 ; true
	@$(foreach DIR,$(DIRS), echo --- $(NAME)/$(DIR) && $(SUBMAKE) -C $(DIR) $@ &&) true
	@$(foreach FILE,$(FILES), $(xCP) $(FILE:-%=%) $(DISTROOT)/Apps/AxWin/3.0/$(FILE:-%=%) &&) true

clean:
	@$(foreach DIR,$(DIRS), $(SUBMAKE) -C $(DIR) $@ &&) true

$(FILES): -%:
	$(xCP) $* $(DISTROOT)/Apps/AxWin/3.0/$*
