#
# Copyright (C) 2009-2011 The Android-x86 Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#

ifneq ($(TARGET_BUILD_VARIANT), user)
KERNEL_DEFCONFIG ?= $(strip $(TARGET_DEVICE))_debug_defconfig
else
KERNEL_DEFCONFIG ?= $(strip $(TARGET_DEVICE))_defconfig
endif

KERNEL_DIR := $(call my-dir)
ROOTDIR := $(abspath $(TOP))

KERNEL_OUT := $(TARGET_OUT_INTERMEDIATES)/KERNEL_OBJ
TARGET_PREBUILT_KERNEL := $(KERNEL_OUT)/arch/arm/boot/zImage
#TARGET_PREBUILT_KERNEL_BIN := $(KERNEL_OUT)/arch/arm/boot/zImage.bin
TARGET_KERNEL_CONFIG := $(KERNEL_OUT)/.config
KERNEL_HEADERS_INSTALL := $(KERNEL_OUT)/usr
KERNEL_MODULES_OUT := $(TARGET_OUT)/lib/modules
KERNEL_MODULES_SYMBOLS_OUT := $(PRODUCT_OUT)/symbols/system/lib/modules

ifeq ($(KERNEL_CROSS_COMPILE),)
KERNEL_CROSS_COMPILE := arm-eabi-
endif

define mv-modules
mdpath=`find $(1) -type f -name modules.dep`;\
if [ "$$mdpath" != "" ];then\
mpath=`dirname $$mdpath`;\
ko=`find $$mpath/kernel -type f -name *.ko`;\
for i in $$ko; do mv $$i $(1)/; done;\
fi
endef

define clean-module-folder
rm -rf $(1)/lib
endef

$(KERNEL_OUT):
	mkdir -p $@

$(KERNEL_MODULES_OUT):
	mkdir -p $@

.PHONY: kernel kernel-defconfig kernel-menuconfig kernel-modules clean-kernel
kernel-menuconfig: | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) menuconfig

kernel-savedefconfig: | $(KERNEL_OUT)
	cp $(TARGET_KERNEL_CONFIG) $(KERNEL_DIR)/arch/arm/configs/$(KERNEL_DEFCONFIG)

$(TARGET_PREBUILT_KERNEL): kernel

$(TARGET_KERNEL_CONFIG) kernel-defconfig: $(KERNEL_DIR)/arch/arm/configs/$(KERNEL_DEFCONFIG) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) $(KERNEL_DEFCONFIG)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) oldconfig

$(KERNEL_HEADERS_INSTALL): $(TARGET_KERNEL_CONFIG) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) headers_install

kernel: $(TARGET_KERNEL_CONFIG) $(KERNEL_HEADERS_INSTALL) | $(KERNEL_OUT)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE)
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) modules
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) INSTALL_MOD_PATH=$(ROOTDIR)/$(KERNEL_MODULES_SYMBOLS_OUT) modules_install
	$(call mv-modules,$(ROOTDIR)/$(KERNEL_MODULES_SYMBOLS_OUT))
	$(call clean-module-folder,$(ROOTDIR)/$(KERNEL_MODULES_SYMBOLS_OUT))
	$(MAKE) -C $(KERNEL_DIR) O=$(ROOTDIR)/$(KERNEL_OUT) ARCH=arm CROSS_COMPILE=$(KERNEL_CROSS_COMPILE) INSTALL_MOD_PATH=$(ROOTDIR)/$(KERNEL_MODULES_OUT) INSTALL_MOD_STRIP=1 modules_install
	$(call mv-modules,$(ROOTDIR)/$(KERNEL_MODULES_OUT))
	$(call clean-module-folder,$(ROOTDIR)/$(KERNEL_MODULES_OUT))

kernel-modules: kernel | $(KERNEL_MODULES_OUT)

$(INSTALLED_KERNEL_TARGET): kernel

#$(TARGET_PREBUILT_KERNEL_BIN): $(TARGET_PREBUILT_KERNEL) | $(HOST_OUT_EXECUTABLES)/mkimage
#	$(HOST_OUT_EXECUTABLES)/mkimage $< KERNEL 0xffffffff > $@ 	

$(INSTALLED_KERNEL_TARGET): $(TARGET_PREBUILT_KERNEL) | $(ACP)
	$(copy-file-to-target)

systemimage: kernel-modules

clean-kernel:
	@rm -rf $(KERNEL_OUT)
	@rm -rf $(KERNEL_MODULES_OUT)
