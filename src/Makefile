#$Id$

SUBDIRS = utils writers teld camd plan

all:
	for dir in $(SUBDIRS) ; do 		\
		$(MAKE) -C $$dir;		\
		if [ $$? -ne "0" ]; 		\
			then exit; 		\
		fi;				\
	done

clean:
	for dir in $(SUBDIRS) ; do		\
		$(MAKE) -C $$dir clean;		\
		if [ $$? -ne "0" ]; 		\
			then exit; 		\
		fi;				\
	done				
	$(RM) *~ core

mrproper:
	for dir in $(SUBDIRS) ; do 		\
		$(MAKE) -C $$dir mrproper;	\
		if [ $$? -ne "0" ]; 		\
			then exit; 		\
		fi;				\
	done
	$(RM) *~ core

indent:
	for dir in $(SUBDIRS) ; do 		\
		$(MAKE) -C $$dir indent;	\
		if [ $$? -ne "0" ]; 		\
			then exit; 		\
		fi;				\
	done

commit: indent
	$(CVS) commit
