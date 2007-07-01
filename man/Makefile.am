# $Id$

man_MANS = \
    rts2.7 \
    rts2.ini.5 \
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
    rts2-image.1 \
    rts2-tpm.1

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

rts2.7: rts2.xml
	$(XP) $(DB2MAN) $<

rts2.ini.5: rts2.ini.xml
	$(XP) $(DB2MAN) $<

rts2-mon.1: rts2-mon.xml
	$(XP) $(DB2MAN) $<

rts2-target.1: rts2-target.xml
	$(XP) $(DB2MAN) $<

rts2-targetinfo.1: rts2-targetinfo.xml
	$(XP) $(DB2MAN) $<

rts2-focusc.1: rts2-focusc.xml
	$(XP) $(DB2MAN) $<

rts2-xfocusc.1: rts2-xfocusc.xml
	$(XP) $(DB2MAN) $<

rts2-seltest.1: rts2-seltest.xml
	$(XP) $(DB2MAN) $<

rts2.script.5: rts2.script.xml
	$(XP) $(DB2MAN) $<

rts2.fitskeys.5: rts2.fitskeys.xml
	$(XP) $(DB2MAN) $<

rts2-horizon.1: rts2-horizon.xml
	$(XP) $(DB2MAN) $<

rts2-scriptexec.1: rts2-scriptexec.xml
	$(XP) $(DB2MAN) $<

rts2-logger.1: rts2-logger.xml
	$(XP) $(DB2MAN) $<

rts2-image.1: rts2-image.xml
	$(XP) $(DB2MAN) $<

rts2-tpm.1: rts2-tpm.xml
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
