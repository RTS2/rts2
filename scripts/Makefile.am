bin_SCRIPTS = rts2-queue rts2-json rts2-bb-json rts2-astrometry.net rts2-sextractor rts2-log \
	imgp_analysis.py rts2-focusing gpoint mosaic-combine satvis rts2-bsc-wcs rts2-build-model-verify \
	rts2-build-model-tool rts2-verify-tracking rts2-fits2gpoint rts2-start rts2-stop upoint2gpoint \
	rts2-guide rts2-scat dump-sitech rts2-test rts2-mpec-ephems rts2-cube

EXTRA_DIST = flat.py guide.py guide-altaz.py masterflat.py center.py match.py systemtest.py \
	rts2-queue rts2-json rts2-astrometry.net rts2-sextractor imgp_analysis.py rts2-focusing \
	acquire.py \
	rts2-queue rts2-json rts2-bb-json rts2-astrometry.net rts2-log subtract.py \
	plot-model.py rts2-parse-bsc rts2-start.in rts2-stop.in

# fully expand variables in rts2-start
do_subst = $(SED) \
		-e 's|[@]bindir@|$(bindir)|g' \
		-e 's|[@]exec_prefix@|$(exec_prefix)|g' \
		-e 's|[@]prefix@|$(prefix)|g' \
		-e 's|[@]CONFDIR@|$(CONFDIR)|g' \
		-e 's|[@]LOG_FILE@|$(LOG_FILE)|g' \
		-e 's|[@]LOG_PREFIX@|$(LOG_PREFIX)|g' \
		-e 's|[@]LOCK_PREFIX@|$(LOCK_PREFIX)|g' \
		-e 's|[@]CENTRALD_PORT@|$(CENTRALD_PORT)|g'

rts2-start:rts2-start.in
	$(do_subst) < $< > $@
	chmod a+x $@

rts2-stop:rts2-stop.in
	$(do_subst) < $< > $@
	chmod a+x $@

SUBDIRS = rts2saf
