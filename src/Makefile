#$Id$
SUBDIRS = utils teld camd
all:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir;	\
	done

clean:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir clean;	\
	done
	$(RM) *~ core

indent:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir indent;	\
	done

commit: indent
	cvs commit
