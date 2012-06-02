NOSTDINC_FLAGS := -I$(srctree)/include/compat/ \
        -include $(srctree)/include/compat/linux/compat-2.6.h \
        $(CFLAGS)

obj-$(CONFIG_CFG80211) += cfg80211.o

obj-$(CONFIG_WEXT_CORE) += wext-core.o
obj-$(CONFIG_WEXT_PROC) += wext-proc.o
obj-$(CONFIG_WEXT_SPY) += wext-spy.o
obj-$(CONFIG_WEXT_PRIV) += wext-priv.o

cfg80211-y += core.o sysfs.o radiotap.o util.o reg.o scan.o nl80211.o
cfg80211-y += mlme.o ibss.o sme.o chan.o ethtool.o mesh.o
cfg80211-$(CONFIG_CFG80211_DEBUGFS) += debugfs.o
cfg80211-$(CONFIG_CFG80211_WEXT) += wext-compat.o wext-sme.o
cfg80211-$(CONFIG_CFG80211_INTERNAL_REGDB) += regdb.o

ccflags-y += -D__CHECK_ENDIAN__

$(obj)/regdb.c: $(src)/db.txt $(src)/genregdb.awk
	@$(AWK) -f $(src)/genregdb.awk < $< > $@

clean-files := regdb.c
