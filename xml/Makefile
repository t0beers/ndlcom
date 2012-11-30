# this is just a simple, non-functional makefile for use during testing. not
# kept up-to-date after initial writage. ONLY use it if you know what you are
# doing!
all: DeviceIds.success Devices.vhd DeviceIds.h DeviceIds.c

%.vhd: %.vhd.xsl DeviceIds.xml
	xmlstarlet tr $*.vhd.xsl DeviceIds.xml > $@

%.h: %.h.xsl %.xml
	xmlstarlet tr $*.h.xsl $*.xml > $@

%.c: %.c.xsl %.xml
	xmlstarlet tr $*.c.xsl $*.xml > $@

%.success: %.xsd %.xml
	@echo "WARNING: only use this makefile if you know what you are doing. you have been warned!"
	xmllint --noout --schema $*.xsd $*.xml && touch $@

clean:
	rm -f DeviceIds.success Devices.vhd DeviceIds.h DeviceIds.c
