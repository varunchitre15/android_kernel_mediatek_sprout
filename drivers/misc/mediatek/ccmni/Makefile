ifneq ($(CONFIG_MTK_CCCI_EXT), y)
ifeq (y,$(CONFIG_MTK_NET_CCMNI))
obj-m	+= ccmni.o
ccmni-$(CONFIG_MTK_NET_CCMNI) := ccmni_pfp.o ccmni_net.o
else
obj-n += dummy.o
endif

else 
obj- :=dummy.o
endif

