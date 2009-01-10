doc: doxyfile
	-doxygen doxyfile

CLEANFILES = html man

EXTRA_DIST = doxyfile.in

# copy to SF project pages
copy_to_sf: doc
	$(MAKE) -C latex
	rsync -av --rsh=ssh html web.sf.net:/home/groups/r/rt/rts-2/htdocs/doc
	rsync -av --rsh=ssh latex/refman.pdf web.sf.net:/home/groups/r/rt/rts-2/htdocs/doc
