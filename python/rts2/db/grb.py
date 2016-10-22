from sqlalchemy import Column, ForeignKey
from sqlalchemy.types import Integer, String, Float, Boolean, DateTime, Text

from rts2.db import Base

class Grb(Base):
	__tablename__ = 'grb'

	tar_id = Column(Integer, ForeignKey('targets.tar_id'), nullable=False)
	grb_id = Column(Integer, primary_key=True, nullable=False)
	grb_seqn = Column(Integer, primary_key=True, nullable=False)
	grb_type = Column(Integer, primary_key=True, nullable=False)
	grb_ra = Column(Float(Precision=64))
	grb_dec = Column(Float(Precision=64))
	grb_is_grb = Column(Boolean, nullable=False)
	grb_date = Column(DateTime, nullable=False)
	grb_last_update = Column(DateTime, nullable=False)
	grb_errorbox = Column(Float(Precision=64))
	grb_autodisabled = Column(Boolean, nullable=False)

	def __init__(self, tar_id=None, grb_ra=None, grb_dec=None, grb_is_grb=None, grb_date=None, grb_last_update=None, grb_errorbox=None, grb_autodisabled=None):
		self.tar_id = tar_id
		self.grb_ra = grb_ra
		self.grb_dec = grb_dec
		self.grb_is_grb = grb_is_grb
		self.grb_date = grb_date
		self.grb_last_update = grb_last_update
		self.grb_errorbox = grb_errorbox
		self.grb_autodisabled = grb_autodisabled

	def __repr__(self):
		return '<Grb({0},{1},{2},{3})>'.format(self.tar_id, self.grb_id, self.grb_seqn, self.grb_type)
