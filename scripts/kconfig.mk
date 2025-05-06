# Warn if no .config exists
ifeq ($(wildcard .config),)
  $(warning Warning: .config does not exist!)
  $(warning Please run 'make menuconfig' first.)
endif

# Paths (relative to project root)
KCONFIG_PATH := tools/kconfig
KCONFIG_FILE := ./Kconfig

# Front-end executables, built under tools/kconfig/build/
CONF  := $(KCONFIG_PATH)/build/conf
MCONF := $(KCONFIG_PATH)/build/mconf

.PHONY: menuconfig savedefconfig defconfig help

# ------------------------------------------------------------------------------
# Build conf and mconf via the in-tree Makefile (supports NAME=conf|mconf)
# ------------------------------------------------------------------------------
$(CONF):
	@$(MAKE) -s -C $(KCONFIG_PATH) NAME=conf

$(MCONF):
	@$(MAKE) -s -C $(KCONFIG_PATH) NAME=mconf

# ------------------------------------------------------------------------------
# menuconfig: run mconf to edit .config, then sync via conf
# ------------------------------------------------------------------------------
menuconfig: $(MCONF) $(CONF)
	@$(MCONF) $(KCONFIG_FILE)
	@$(CONF)  --syncconfig $(KCONFIG_FILE)

# ------------------------------------------------------------------------------
# savedefconfig: generate a minimal defconfig under configs/defconfig
# ------------------------------------------------------------------------------
savedefconfig: $(CONF)
	@$(CONF) --savedefconfig=configs/defconfig $(KCONFIG_FILE)

# ------------------------------------------------------------------------------
# %defconfig: load configs/<name>defconfig and sync to .config
# ------------------------------------------------------------------------------
%defconfig: $(CONF)
	@$(CONF) --defconfig=configs/$@ $(KCONFIG_FILE)
	@$(CONF) --syncconfig $(KCONFIG_FILE)

# ------------------------------------------------------------------------------
# help: display available Kconfig commands
# ------------------------------------------------------------------------------
help:
	@echo "  menuconfig       - launch the menu-based config editor"
	@echo "  savedefconfig    - save minimal config to configs/defconfig"
	@echo "  <name>defconfig  - load configs/<name>defconfig and sync"
