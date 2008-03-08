doc: doxyfile
	-doxygen doxyfile

CLEANFILES = html man

EXTRA_DIST = doxyfile.in

# copy to SF project pages
copy_to_sf: doc
	$(MAKE) -C latex
	ssh shell.sf.net rm -rf /home/groups/r/rt/rts-2/htdocs/doc/html
	scp -rC html latex/refman.pdf shell.sf.net:/home/groups/r/rt/rts-2/htdocs/doc
