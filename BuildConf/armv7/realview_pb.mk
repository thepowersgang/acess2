
include $(ACESSDIR)/BuildConf/armv7/default.mk

ARM_CPUNAME = cortex-a8
MODULES += armv7/GIC
MODULES += Input/PS2KbMouse
MODULES += Display/PL110
