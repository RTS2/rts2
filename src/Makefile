#$Id$

SUBDIRS = utils writers teld camd plan

all:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir;	\
	done

clean:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir clean;	\
	done
	$(RM) *~ core

mrproper:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir mrproper;	\
	done
	$(RM) *~ core

indent:
	for dir in $(SUBDIRS) ; do 	\
		$(MAKE) -C $$dir indent;	\
	done

commit: indent
	$(CVS) commit
