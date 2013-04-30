#!/usr/bin/env python

from sqlalchemy import create_engine
from sqlalchemy.orm.session import sessionmaker
from rts2.db import Targets

Session = sessionmaker()
engine = create_engine('postgresql://petr:petr@localhost/stars',echo='debug')
Session.configure(bind=engine)

sess = Session()

q = sess.query(Targets)
#q = sess.query(ApacheCatalog)
data = q.filter(Targets.tar_id < 1000).all()
print data
