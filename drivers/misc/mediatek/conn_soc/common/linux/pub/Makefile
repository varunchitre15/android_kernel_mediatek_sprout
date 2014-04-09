ifeq ($(CONFIG_MTK_COMBO), y)
ccflags-y += -I$(src)/../../linux/include \
						 -I$(src)/../../linux/pri/include \
						 -I$(src)/../../core/include \
						 -I$(src)/../../include \
						 -I$(src)/../ \
						 -I$(src)/../../../../common_detect
						 
ccflags-y += -I$(MTK_CUSTOM_PATH)/kernel/drivers/eemcs/
ccflags-y += -I$(MTK_CUSTOM_PATH)/kernel/drivers/conn_md/include
ccflags-y += -DWMT_CREATE_NODE_DYNAMIC=1

obj-y += osal.o \
         bgw_desense.o \
         stp_chrdev_bt.o \
         stp_chrdev_gps.o \
         wmt_chrdev_wifi.o \
         wmt_idc.o

endif
