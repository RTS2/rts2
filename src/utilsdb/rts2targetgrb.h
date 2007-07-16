#ifndef __RTS2_TARGETGRB__
#define __RTS2_TARGETGRB__

#include "target.h"

class ConstTarget;

class TargetGRB:public ConstTarget
{
private:
  double grbDate;
  double lastUpdate;
  int gcnPacketType;		// gcn packet from last update
  int gcnGrbId;
  bool grb_is_grb;
  int shouldUpdate;
  int gcnPacketMin;		// usefull for searching for packet class
  int gcnPacketMax;

  int maxBonusTimeout;
  int dayBonusTimeout;
  int fiveBonusTimeout;

  struct ln_equ_posn grb;
  double errorbox;

  const char *getSatelite ();
protected:
    virtual int getDBScript (const char *camera_name, std::string & script);
public:
    TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs,
	       int in_maxBonusTimeout, int in_dayBonusTimeout,
	       int in_fiveBonusTimeout);
  virtual int load ();
  virtual int getPosition (struct ln_equ_posn *pos, double JD);
  virtual int compareWithTarget (Target * in_target, double grb_sep_limit);
  virtual int getScript (const char *deviceName, std::string & buf);
  virtual int beforeMove ();
  virtual float getBonus (double JD);
  virtual double getMinAlt ()
  {
    return 0;
  }
  // some logic needed to distinguish states when GRB position change
  // from last observation. there was update etc..
  virtual int isContinues ();

  double getFirstPacket ();
  virtual void printExtra (std::ostream & _os, double JD);

  virtual void printDS9Reg (std::ostream & _os, double JD);

  void printGrbList (std::ostream & _os);

  virtual void writeToImage (Rts2Image * image);
};

#endif /* !__RTS2_TARGETGRB__ */
