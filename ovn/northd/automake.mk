# ovn-northd
bin_PROGRAMS += ovn/northd/ovn-northd
ovn_northd_ovn_northd_SOURCES = \
	ovn/northd/ovn-northd.c \
	ovn/northd/chassis_sync.c \
	ovn/northd/chassis_sync.h
ovn_northd_ovn_northd_LDADD = \
	ovn/lib/libovn.la \
	ovsdb/libovsdb.la \
	lib/libopenvswitch.la
man_MANS += ovn/northd/ovn-northd.8
EXTRA_DIST += ovn/northd/ovn-northd.8.xml
DISTCLEANFILES += ovn/northd/ovn-northd.8
