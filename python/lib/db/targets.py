from sqlalchemy import Column, ForeignKey
from sqlalchemy.types import Integer, String, Float, Boolean, DateTime, Text

from rts2.db import Base

class Targets(Base):
	__tablename__ = 'targets'

	tar_id = Column(Integer, primary_key=True)
	tar_name = Column(String(150))
	tar_ra = Column(Float(Precision=64))
	tar_dec = Column(Float(Precision=64))
	tar_comment = Column(Text)
	tar_enabled = Column(Boolean)
	tar_priority = Column(Integer)
	tar_bonus = Column(Integer)
	tar_bonus_time = Column(DateTime)
	tar_next_observable = Column(DateTime)
	tar_info = Column(String(2000))
	interruptible = Column(Boolean)
	tar_pm_ra = Column(Float(Precision=64))
	tar_pm_dec = Column(Float(Precision=64))
	tar_telescope_mode = Column(Integer)

	def __init__(self, tar_name=None, tar_ra=None, tar_dec=None, tar_comment=None, tar_enabled=None, tar_priority=None, tar_bonus=None, tar_bonus_time=None, tar_next_observable=None, tar_info=None, interruptible=None, tar_pm_ra=None, tar_pm_dec=None, tar_telescope_mode=None):
		self.tar_name = tar_name
		self.tar_ra = tar_ra
		self.tar_dec = tar_dec
		self.tar_comment = tar_comment
		self.tar_enabled = tar_enabled
		self.tar_priority = tar_priority
		self.tar_bonus = tar_bonus
		self.tar_bonus_time = tar_bonus_time
		self.tar_next_observable = tar_next_observable
		self.tar_info = tar_info
		self.interruptible = interruptible
		self.tar_pm_ra = tar_pm_ra
		self.tar_pm_dec = tar_pm_dec
		self.tar_telescope_mode = tar_telescope_mode
	
	def __repr__(self):
		return '<Target({0},{1},{2},{3})>'.format(self.tar_id, self.tar_name, self.tar_ra, self.tar_dec)
