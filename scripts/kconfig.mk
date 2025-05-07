# Paths (relative to project root)
KCONFIG_PATH := tools/kconfig
KCONFIG_FILE := ./Kconfig

# Front-end executables, built under tools/kconfig/build/
CONF  := $(KCONFIG_PATH)/build/conf
MCONF := $(KCONFIG_PATH)/build/mconf

# Build conf and mconf via the in-tree Makefile (supports NAME=conf|mconf)
$(CONF):
	@$(MAKE) -s -C $(KCONFIG_PATH) NAME=conf

$(MCONF):
	@$(MAKE) -s -C $(KCONFIG_PATH) NAME=mconf

# menuconfig: run mconf to edit .config, then sync via conf
menuconfig: $(MCONF) $(CONF)
	@$(MCONF) $(KCONFIG_FILE)
	@$(CONF)  --syncconfig $(KCONFIG_FILE)

.PHONY: menuconfig
