# $Id$

SUBDIRS = dia fitsdb

docdir = ${datadir}/doc/@PACKAGE@/doc

dist_noinst_DATA = article_spain.tex	\
		authorizace.tex		\
		bootes.tex		\
		cas.tex			\
		client-db.tex		\
		client-focus.tex	\
		client-grb.tex		\
		client-imagining.tex	\
		client-nmonitor.tex	\
		client-plan.tex		\
		client.tex		\
		db.tex			\
		design.tex		\
		device.tex		\
		diplomka.tex		\
		gui.tex			\
		hw-kamera.tex		\
		ide.tex			\
		implementace.tex	\
		interface.tex		\
		komunikace.tex		\
		konfigurace.tex		\
		linear.tex		\
		macros.tex		\
		nightview.tex		\
		ondrejov-60.tex		\
		pointace.tex		\
		priority.tex		\
		protokoly.tex		\
		prvni.tex		\
		refs.tex		\
		rts-1-zadani.tex	\
		rts-1.tex		\
		server-camd.tex		\
		server-central.tex	\
		server-client.tex	\
		server-design.tex	\
		server-dome.tex		\
		server-protokol.tex	\
		server-serverd.tex	\
		server-teld.tex		\
		server.tex		\
		snimky.tex		\
		srovnani.tex		\
		stavy.tex		\
		uvod.tex		\
		uzivatel.tex		\
		zadani.tex		\
		zaver.tex

EXTRA_DIST = mff.pdf nmonitor.pdf

nodist_doc_DATA = diplomka.pdf

diplomka.pdf:
		pdfcslatex diplomka.tex
		$(RM) diplomka.pdf
		pdfcslatex diplomka.tex

