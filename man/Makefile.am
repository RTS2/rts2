# $Id$

EXTRA_DIST = \
    rts2.xml \
    rts2.ini.xml \
    rts2-device.xml \
    rts2-mon.xml \
    rts2-target.xml \
    rts2-targetinfo.xml \
    rts2-focusc.xml \
    rts2-xfocusc.xml \
    rts2-seltest.xml \
    rts2.script.xml \
    rts2.fitskeys.xml \
    rts2-horizon.xml \
    rts2-mon.xml \
    rts2-scriptexec.xml \
    rts2-logger.xml \
    rts2-logd.xml \
    rts2-image.xml \
    rts2-tpm.xml \
    rts2-nightreport.xml \
    rts2-scriptor.xml \
    rts2-grbd.xml \
    rts2-user.xml \
    rts2-telmodeltest.xml \
    rts2.devices.xml \
    rts2-centrald.xml \
    rts2-moodd.xml \
    rts2-sensor-system.xml \
    rts2-sensor-apcups.xml \
    rts2-camd.xml

man_MANS = \
    rts2.7 \
    rts2.ini.5 \
    rts2-device.7 \
    rts2-mon.1 \
    rts2-target.1 \
    rts2-targetinfo.1 \
    rts2-focusc.1 \
    rts2-xfocusc.1 \
    rts2-seltest.1 \
    rts2.script.5 \
    rts2.fitskeys.5 \
    rts2-horizon.1 \
    rts2-mon.1 \
    rts2-scriptexec.1 \
    rts2-logger.1 \
    rts2-logd.1 \
    rts2-image.1 \
    rts2-tpm.1 \
    rts2-nightreport.1 \
    rts2-scriptor.1 \
    rts2-grbd.1 \
    rts2-user.1 \
    rts2-telmodeltest.1 \
    rts2.devices.5 \
    rts2-centrald.1 \
    rts2-moodd.1 \
    rts2-sensor-system.1 \
    rts2-sensor-apcups.1 \
    rts2-camd.7

doc_FOP = \
    rts2-man.fop \
    rts2-book.fop

doc_PDF = \
    rts2-man.pdf \
    rts2-book.pdf

XP=xsltproc -''-nonet

DB2MAN=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl

DB2FOP=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/fo/docbook.xsl
FOP=fop

%.1: %.xml
	$(XP) $(DB2MAN) $<

%.5: %.xml
	$(XP) $(DB2MAN) $<

%.7: %.xml
	$(XP) $(DB2MAN) $<

# now FOP & related activites..

rts2-man.fop: rts2-mon.xml
	$(XP) $(DB2FOP) $< > $@

rts2-man.pdf: rts2-man.fop
	$(FOP) $< $@

rts2-book.fop: rts2-book.xml
	$(XP) $(DB2FOP) $< > $@

rts2-book.pdf: rts2-book.fop
	$(FOP) $< $@

CLEANFILES = *.1 *.5 *.7
