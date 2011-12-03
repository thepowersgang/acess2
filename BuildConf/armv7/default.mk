
ifeq ($(PLATFORM),default)
	$(error Please select a platform)
endif

# Core ARMv7 modules

MODULES += armv7/GIC
MODULES += Filesystems/InitRD
