
ifeq ($(CONFIG),default)
	$(error Please select a configuration)
endif

MODULES += armv7/GIC
MODULES += Filesystems/InitRD
