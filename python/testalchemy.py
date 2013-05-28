#!/usr/bin/env python

from sqlalchemy import create_engine, and_, or_
from sqlalchemy.orm.session import sessionmaker
from rts2.db import Targets,Grb

Session = sessionmaker()
engine = create_engine('postgresql://petr:petr@localhost/stars',echo='debug')
Session.configure(bind=engine)

sess = Session()

targ = sess.query(Targets)
#q = sess.query(ApacheCatalog)
print targ.filter(Targets.tar_id == 1000).all()

print targ.filter(and_(Targets.tar_ra < 20, Targets.tar_dec < 0, Targets.tar_dec > -20)).all()

grb = sess.query(Grb)

print grb.filter(Grb.tar_id == 50001).all()
