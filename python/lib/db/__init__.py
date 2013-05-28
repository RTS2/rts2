from sqlalchemy.ext.declarative import declarative_base
Base = declarative_base()

from rts2.db.targets import Targets
from rts2.db.grb import Grb
