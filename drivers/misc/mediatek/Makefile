#$(call all-subdir-src-or-makefile)

include $(srctree)/drivers/misc/mediatek/Makefile.custom

obj-y	+= kernel/
obj-y	+= aee/
obj-y	+= dual_ccci/
obj-y	+= fmradio/
obj-y	+= gps/
obj-y	+= hwmon/
obj-y	+= m4u/
obj-y	+= mhl/
obj-y	+= mtprof/
obj-y   += partition/

obj-y	+= sched/
obj-y	+= video/
ifneq ($(MTK_PROJECT), mt6589fpga_ca7_ldvt mt6582fpga_ldvt)

endif
obj-y	+= bthid/
obj-y	+= ccmni/
obj-y	+= emd_ctl/

obj-y	+= $(call all-subdirs-with-makefile,gator-driver)
obj-y	+= $(call all-subdirs-with-makefile,met)

#obj-y   +=  hdmitx/
obj-y	+= mmp/
obj-y	+= ram_console/
obj-y   += thermal/
obj-y   += videocodec/
obj-y   += xlog/
obj-y   += wdk/
obj-y	+= mt_logger/

obj-y	+= alsps/
obj-y	+= accelerometer/
obj-y	+= gyroscope/
obj-y	+= magnetometer/
obj-y	+= matv/
#obj-y	+= touchpanel/
#obj-y	+= cpu_stress/


ifneq ($(CONFIG_MTK_COMBO),)
	subdir-ccflags-y += -D MTK_WCN_WMT_STP_EXP_SYMBOL_ABSTRACT
	subdir-ccflags-y += -D MTK_WCN_REMOVE_KERNEL_MODULE
	subdir-ccflags-y += -D MTK_CUSTOM_PATH=$(srctree)/drivers/misc/mediatek/

	ifneq ($(filter "MT6620E3",$(CONFIG_MTK_COMBO_CHIP)),)
		obj-y   +=  combo/
	endif
	ifneq ($(filter "MT6628",$(CONFIG_MTK_COMBO_CHIP)),)
		obj-y   +=  combo/
	endif
	ifneq ($(filter "CONSYS_6572",$(CONFIG_MTK_COMBO_CHIP)),)
#		$(warning Haman compile 72 $(CONFIG_MTK_COMBO_CHIP))
		obj-y   +=  conn_soc/
	endif
	ifneq ($(filter "CONSYS_6582",$(CONFIG_MTK_COMBO_CHIP)),)
#		$(warning Haman compile 82 $(CONFIG_MTK_COMBO_CHIP))
		obj-y   +=  conn_soc/
	endif
	obj-y += common_detect/
endif

#obj-y += conn_md/
obj-$(CONFIG_MTK_BTIF)	+= btif/

obj-y	+= ion_drv/
ifeq ($(CONFIG_MTK_NFC_SUPPORT),y)
obj-y	+= nfc/
endif

obj-$(CONFIG_ZRAM)	+= mlog/
obj-y	+= cam_cal/dummy_eeprom/
obj-y	+= lens/
obj-y	+= imgsensor/src/
obj-y   += flashlight/src/
obj-y	+= sync/

#rtc must compile before power, because they are both in fs_initcall, rtc need initial first for PMIC alarm call back
obj-y	+= rtc/
obj-y	+= boot/
obj-y	+= gpio/
obj-y	+= pwm/
obj-y	+= eint/
obj-y	+= uart/
obj-y	+= pmic_wrap/
obj-y	+= hibcore/
ifeq ($(CONFIG_CUSTOM_KERNEL_FLASHLIGHT),"dummy_flashlight")
obj-y   += flashlight/dummy_flashlight/
endif
obj-y	+= auxadc/
obj-y	+= accdet/
obj-y	+= keypad/
obj-y	+= dum-char/
obj-y	+= ssw/
obj-$(CONFIG_MTK_CMMB) += cmmbdrv/
obj-y	+= devapc/
obj-y	+= devinfo/
obj-y	+= dispsys/
obj-y	+= gpu/
obj-y	+= i2c/
obj-y	+= leds/
obj-y	+= masp/
obj-$(CONFIG_MTK_MMC) += mmc-host/
#obj-$(CONFIG_MTK_ETM) += mt_etm/
obj-$(CONFIG_MTK_MTD_NAND) += nand/

obj-y   += dbg_dump/
obj-y	+= power/
obj-y	+= power_gs/
obj-y	+= pwm/
obj-y	+= smi/
obj-y	+= btcvsd/
obj-y	+= sound/
obj-y	+= spi/
obj-y	+= usb20/
obj-y	+= vibrator/
obj-y	+= wdt/
obj-y	+= freqhopping/
obj-y	+= mrdump/
obj-y   += selinux_warning/

#charge pump
obj-y	+= chargepump/
