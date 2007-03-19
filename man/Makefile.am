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
    rts2.fitskeys.5 

DB2MAN=/usr/share/sgml/docbook/stylesheet/xsl/nwalsh/manpages/docbook.xsl
XP=xsltproc -''-nonet

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

CLEANFILES = *.1 *.5 *.7
