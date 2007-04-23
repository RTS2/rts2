#ifndef __RTS2_SCRIPT_ELEMENT_ACQUIRE__
#define __RTS2_SCRIPT_ELEMENT_ACQUIRE__

#include "rts2script.h"
#include "rts2scriptelement.h"

class Rts2ScriptElementAcquire:public Rts2ScriptElement
{
protected:
  double reqPrecision;
  double lastPrecision;
  float expTime;
  enum
  { NEED_IMAGE, WAITING_IMAGE, WAITING_ASTROMETRY, WAITING_MOVE, PRECISION_OK,
    PRECISION_BAD, FAILED
  } processingState;
  char defaultImgProccess[2000];
  int obsId;
  int imgId;
  struct ln_equ_posn center_pos;
public:
    Rts2ScriptElementAcquire (Rts2Script * in_script, double in_precision,
			      float in_expTime,
			      struct ln_equ_posn *in_center_pos);
  virtual void postEvent (Rts2Event * event);
  virtual int nextCommand (Rts2DevClientCamera * camera,
			   Rts2Command ** new_command,
			   char new_device[DEVICE_NAME_SIZE]);
  virtual int processImage (Rts2Image * image);
  virtual void cancelCommands ();
};

/** 
 * Class for bright source acquistion
 */
class Rts2ScriptElementAcquireStar:public Rts2ScriptElementAcquire
{
private:
  int maxRetries;
  int retries;
  double spiral_scale_ra;
  double spiral_scale_dec;
  double minFlux;
  Rts2Spiral *spiral;
protected:
  /**
   * Decide, if image contains source of interest...
   *
   * It's called from processImage to decide what to do.
   *
   * @return -1 when we should continue in spiral search, 0 when source is in expected position,
   * 1 when source is in field, but offset was measured; in that case it fills ra_offset and dec_offset.
   */
    virtual int getSource (Rts2Image * image, double &ra_off,
			   double &dec_off);
public:
  /**
   * @param in_spiral_scale_ra  RA scale in degrees
   * @param in_spiral_scale_dec DEC scale in degrees
   */
    Rts2ScriptElementAcquireStar (Rts2Script * in_script, int in_maxRetries,
				  double in_precision, float in_expTime,
				  double in_spiral_scale_ra,
				  double in_spiral_scale_dec,
				  struct ln_equ_posn *in_center_pos);
    virtual ~ Rts2ScriptElementAcquireStar (void);
  virtual void postEvent (Rts2Event * event);
  virtual int processImage (Rts2Image * image);
};

/**
 * Some special handling for HAM..
 */
class Rts2ScriptElementAcquireHam:public Rts2ScriptElementAcquireStar
{
private:
  int maxRetries;
  int retries;
protected:
    virtual int getSource (Rts2Image * image, double &ra_off,
			   double &dec_off);
public:
    Rts2ScriptElementAcquireHam (Rts2Script * in_script, int in_maxRetries,
				 float in_expTime,
				 struct ln_equ_posn *in_center_pos);
    virtual ~ Rts2ScriptElementAcquireHam (void);
};

#endif /* !__RTS2_SCRIPT_ELEMENT_ACQUIRE__ */
