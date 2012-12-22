doc: doxyfile
	-doxygen doxyfile

CLEANFILES = html man

EXTRA_DIST = doxyfile.in

# copy to SF project pages
copy_to_sf:
	rsync -av --rsh=ssh html web.sf.net:/home/project-web/rts-2/htdocs/doc/
	rsync -av --rsh=ssh latex/refman.pdf web.sf.net:/home/project-web/rts-2/htdocs/doc/
