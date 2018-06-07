SUBDIRS =	include \
		lib     \
		src	\
		doc	\
		man	\
		contrib \
		python  \
		scripts \
		conf    \
		checks

EXTRA_DIST = autogen.sh RUN rts2.initd.in \
	INSTALL INSTALL.alta INSTALL.fli INSTALL.paramount INSTALL.andor \
	INSTALL.comedi INSTALL.miccd INSTALL.sbig INSTALL.gpib rts2-init.in \
	README.md

README: README.md
	ln -s $< $@

noinst_HEADERS = config.h

ACLOCAL_AMFLAGS = -I m4

spellcheck:
	cd man && make $@
