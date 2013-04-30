#!/usr/bin/env python

from sqlalchemy import create_engine, and_, or_
from sqlalchemy.orm.session import sessionmaker
from rts2.db import Targets

Session = sessionmaker()
engine = create_engine('postgresql://petr:petr@localhost/stars',echo='debug')
Session.configure(bind=engine)

sess = Session()

q = sess.query(Targets)
#q = sess.query(ApacheCatalog)
print q.filter(Targets.tar_id == 1000).all()

print q.filter(and_(Targets.tar_ra < 20, Targets.tar_dec < 0, Targets.tar_dec > -20)).all()
