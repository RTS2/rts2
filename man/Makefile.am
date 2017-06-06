# $Id$

EXTRA_DIST = \
    rts2.conf \
    rts2.ent.in \
    rts2.txt \
    rts2.ini.xml \
    rts2-device.xml \
    rts2-mon.xml \
    rts2-target.xml \
    rts2-targetinfo.xml \
    rts2-targetlist.xml \
    rts2-focusc.xml \
    rts2-xfocusc.xml \
    rts2-seltest.xml \
    rts2.script.xml \
    rts2.fitskeys.xml \
    rts2-horizon.xml \
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
    rts2-sensor-mrakomer.xml \
    rts2-camd.xml \
    rts2-dome.xml \
    rts2-focusd.xml \
    rts2-teld.xml \
    rts2-teld-sitech-gem.xml \
    rts2-telescope-model.xml \
    rts2-simbadinfo.xml \
    rts2-httpd.xml \
    rts2-xmlrpcclient.xml \
    rts2-sensor-aag.xml \
    rts2-talker.xml \
    rts2.target.xml \
    rts2-newtarget.xml \
    rts2-plan.xml \
    rts2-selector.xml \
    rts2-executor.xml \
    rts2-queue.xml \
    rts2.queue.xml \
    rts2-imgproc.xml \
    rts2.fitstemplate.xml \
    rts2.valuefile.xml \
    rts2-camd-reflex.xml \
    rts2pa_measurement.py.xml \
    rts2-bb.xml \
    gpoint.xml

CLEANFILES = *.1 *.5 *.7

if HAVE_DB2MAN

man_MANS = \
    rts2.7 \
    rts2.ini.5 \
    rts2-device.7 \
    rts2-mon.1 \
    rts2-target.1 \
    rts2-targetinfo.1 \
    rts2-targetlist.1 \
    rts2-focusc.1 \
    rts2-xfocusc.1 \
    rts2-seltest.1 \
    rts2.script.5 \
    rts2.fitskeys.5 \
    rts2-horizon.1 \
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
    rts2-sensor-mrakomer.1 \
    rts2-camd.7 \
    rts2-dome.7 \
    rts2-focusd.7 \
    rts2-teld.7 \
    rts2-teld-sitech-gem.7 \
    rts2-telescope-model.7 \
    rts2-simbadinfo.1 \
    rts2-httpd.7 \
    rts2-xmlrpcclient.1 \
    rts2-sensor-aag.1 \
    rts2-talker.1 \
    rts2.target.5 \
    rts2-newtarget.1 \
    rts2-plan.1 \
    rts2-selector.7 \
    rts2-executor.7 \
    rts2-queue.1 \
    rts2.queue.5 \
    rts2-imgproc.7 \
    rts2.fitstemplate.5 \
    rts2.valuefile.5 \
    rts2-camd-reflex.7 \
    rts2pa_measurement.py.1 \
    rts2-bb.7 \
    gpoint.1

XP=xsltproc --nonet

COMPILE_DB2MAN=@DB2MAN@

.xml.1:
	$(XP) $(COMPILE_DB2MAN) $<

.xml.5:
	$(XP) $(COMPILE_DB2MAN) $<

.xml.7:
	$(XP) $(COMPILE_DB2MAN) $<

endif

if HAVE_DB2FOP

doc_FOP = \
    rts2-man.fop \
    rts2-book.fop

COMPILE_DB2FOP=@DB2FOP@
FOP=fop

doc_PDF = \
    rts2-man.pdf \
    rts2-book.pdf

rts2-man.fop: rts2-mon.xml
	$(XP) $(COMPILE_DB2FOP) $< > $@

rts2-man.pdf: rts2-man.fop
	$(FOP) $< $@

rts2-book.fop: rts2-book.xml
	$(XP) $(COMPILE_DB2FOP) $< > $@

rts2-book.pdf: rts2-book.fop
	$(FOP) $< $@

endif

if HAVE_A2X

A2X_MANPAGE_OPTS = --doctype manpage --format manpage \
	--xsltproc-opts "--nonet" \
	--attribute mansource="RTS2" \
	--attribute manversion="@PACKAGE_VERSION@" \
	--attribute manmanual="RTS2 Manual" \
	--destination-dir=.

.txt.1:
	$(A2X) $(A2X_MANPAGE_OPTS) $<

.txt.7:
	$(A2X) $(A2X_MANPAGE_OPTS) $<

else

EXTRA_DIST += rts2.txt

endif
