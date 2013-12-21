
include $(ACESSDIR)/BuildConf/armv7/default.mk

ARM_CPUNAME = cortex-a9
MODULES += armv7/GIC
MODULES += Display/Tegra2Vid
MODULES += USB/Core USB/EHCI
MODULES += USB/HID USB/MSC
