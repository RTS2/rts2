from sqlalchemy import Column, ForeignKey
from sqlalchemy.types import Integer, String, Float

from rts2.db import Base

class Targets(Base):
	__tablename__ = 'targets'

	tar_id = Column(Integer, primary_key=True)
	tar_ra = Column(Float)
	tar_dec = Column(Float)

	def __init__(self, tar_ra=None, tar_dec=None):
		self.tar_ra = tar_ra
		self.tar_dec = tar_dec
	
	def __repr__(self):
		return '<Target({0},{1},{2})>'.format(self.tar_id, self.tar_ra, self.tar_dec)
