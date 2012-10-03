
ifeq ($(PLATFORM),default)
	$(error Please select a platform)
endif

#MODULES += armv7/GIC
MODULES += Filesystems/InitRD
