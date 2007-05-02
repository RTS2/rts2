#ifndef __RTS2_CAMERA_CPP__
#define __RTS2_CAMERA_CPP__

#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"
#include "imghdr.h"

#include "filter.h"

#define MAX_CHIPS  3
#define MAX_DATA_RETRY 100

/* image types, taken from fitsio.h */
#define CAMERA_BYTE_IMG		8
#define CAMERA_SHORT_IMG	16
#define CAMERA_LONG_IMG		32
#define CAMERA_FLOAT_IMG	-32
#define CAMERA_DOUBLE_IMG	-64
#define CAMERA_USHORT_IMG	20
#define CAMERA_ULONG_IMG	40

#define CAMERA_COOL_OFF		0
#define CAMERA_COOL_MAX		1
#define CAMERA_COOL_HOLD	2
#define CAMERA_COOL_SHUTDOWN	3

class Rts2DevCamera;

class ChipSubset
{
public:
  int x, y, width, height;
    ChipSubset (int in_x, int in_y, int in_width, int in_height)
  {
    x = in_x;
    y = in_y;
    width = in_width;
    height = in_height;
  }
  ChipSubset (const ChipSubset * copy)
  {
    x = copy->x;
    y = copy->y;
    width = copy->width;
    height = copy->height;
  }
  int getWidth ()
  {
    return width;
  }
  int getHeight ()
  {
    return height;
  }
};

/**
 * Class holding information about one camera chip.
 *
 * The idea behind is: we will put there size and other things.
 * There aren't any methods which will handle communication with camera;
 * all comunication should be handled directly in device methods.
 */
class CameraChip
{
private:
  void initData (Rts2DevCamera * in_cam, int in_chip_id);

  int sendChip (Rts2Conn * conn, char *name, int value);
  int sendChip (Rts2Conn * conn, char *name, float value);
  int sendChip (Rts2Conn * conn, char *name, double value);
  time_t readout_started;
  int shutter_state;

protected:
  int chipId;
  struct timeval exposureEnd;
  int readoutLine;
  int sendLine;
  Rts2DevConnData *readoutConn;

  ChipSubset *chipSize;
  ChipSubset *chipReadout;
  ChipSubset *chipUsedReadout;

  Rts2DevCamera *camera;

  double pixelX;
  double pixelY;

  int binningVertical;
  int binningHorizontal;
  int usedBinningVertical;
  int usedBinningHorizontal;
  double subExposure;
  int nAcc;
  struct imghdr focusingHeader;

  int sendReadoutData (char *data, size_t data_size);

  char *focusingData;
  char *focusingDataTop;

  virtual int processData (char *data, size_t size);
  /**
   * Function to do focusing. To fullfill it's task, it can
   * use following informations:
   *
   * focusingData is (char *) array holding last image
   * focusingHeader holds informations about image size etc.
   * focusExposure is (float) with exposure setting of focussing
   *
   * Shall focusing need a box image, it should call box method.
   *
   * Return 0 if focusing should continue, !0 otherwise.
   */
  virtual int doFocusing ();

public:
    CameraChip (Rts2DevCamera * in_cam, int in_chip_id);
    CameraChip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
		int in_height, double in_pixelX, double in_pixelY);
    virtual ~ CameraChip (void);
  void setSize (int in_width, int in_height, int in_x, int in_y)
  {
    chipSize = new ChipSubset (in_x, in_y, in_width, in_height);
    chipReadout = new ChipSubset (in_x, in_y, in_width, in_height);
  }
  int getWidth ()
  {
    return chipSize->getWidth ();
  }
  int getHeight ()
  {
    return chipSize->getHeight ();
  }
  virtual int init ();
  int send (Rts2Conn * conn);
  virtual int setBinning (int in_vert, int in_hori);
  virtual int box (int in_x, int in_y, int in_width, int in_height);
  int center (int in_w, int in_h);
  virtual int startExposure (int light, float exptime);
  int setExposure (float exptime, int in_shutter_state);
  virtual long isExposing ();
  virtual int endExposure ();
  virtual int stopExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  void setReadoutConn (Rts2DevConnData * dataConn);
  virtual void deleteConnection (Rts2Conn * conn);
  int getShutterState ()
  {
    return shutter_state;
  }
  virtual int endReadout ();
  void clearReadout ();
  virtual int sendFirstLine ();
  virtual int readoutOneLine ();
  virtual void cancelPriorityOperations ();
  /**
   * If chip support frame transfer.
   *
   * @return false (default) if we don't support frame transfer, so
   * request for readout will be handled on-site, exposure will be
   * handed when readout ends
   */
  virtual bool supportFrameTransfer ();
};

#define NOT_EXP		0x00
#define FILTER_MOVE     0x01
#define FT_EXP		0x02
#define FOCUSER_MOVE	0x04

class Rts2DevCamera:public Rts2Device
{
private:
  char *focuserDevice;
  char *wheelDevice;
  Rts2ValueInteger *nextExp;
  int nextExpChip;
  float nextExpTime;
  int nextExpLight;
  int lastFilterNum;
  Rts2ValueFloat *lastExp;

  int camStartExposure (int chip, int light, float exptime);
  // when we call that function, we must be sure that either filter or wheelDevice != NULL
  int camFilter (int new_filter);
  int camFilter (const char *new_filter);

  double defaultSubExposure;
  Rts2ValueDouble *subExposure;

  Rts2ValueInteger *camFilterVal;
  Rts2ValueInteger *camFocVal;
  Rts2ValueInteger *camShutterVal;

  int getStateChip (int chip_num);
  void maskStateChip (int chip_num, int state_mask, int new_state,
		      char *description);

  int paramNextChip (Rts2Conn * conn, int *in_chip);

protected:
    virtual int processOption (int in_opt);

  int willConnect (Rts2Address * in_addr);
  char *device_file;
  // camera chips
  CameraChip *chips[MAX_CHIPS];
  int chipNum;
  // temperature and others; all in deg Celsius
  Rts2ValueFloat *tempAir;
  Rts2ValueFloat *tempCCD;
  Rts2ValueFloat *tempSet;
  Rts2ValueInteger *tempRegulation;
  Rts2ValueInteger *coolingPower;
  Rts2ValueInteger *fan;

  Rts2ValueInteger *canDF;	// if the camera can make dark frames
  char ccdType[64];
  char *ccdRealType;
  char serialNumber[64];

  Rts2Filter *filter;

  float nightCoolTemp;

  virtual void cancelPriorityOperations ();
  int defBinning;

  Rts2ValueDouble *rnoise;

  virtual int setSubExposure (double in_subexposure);

  virtual void afterReadout ();

  virtual int setValue (Rts2Value * old_value, Rts2Value * new_value);
public:
    Rts2DevCamera (int argc, char **argv);
    virtual ~ Rts2DevCamera (void);

  virtual int initChips ();
  virtual int initValues ();
  void checkExposures ();
  void checkReadouts ();

  virtual void postEvent (Rts2Event * event);

  virtual int idle ();

  virtual int deleteConnection (Rts2Conn * conn)
  {
    for (int i = 0; i < chipNum; i++)
      chips[i]->deleteConnection (conn);
    return Rts2Device::deleteConnection (conn);
  }

  virtual int changeMasterState (int new_state);
  virtual Rts2DevClient *createOtherType (Rts2Conn * conn,
					  int other_device_type);
  virtual int ready ()
  {
    return -1;
  }
  virtual int info ();

  virtual int scriptEnds ();

  virtual int camChipInfo (int chip)
  {
    return -1;
  }
  virtual int camExpose (int chip, int light, float exptime)
  {
    return chips[chip]->startExposure (light, exptime);
  }
  virtual long camWaitExpose (int chip);
  virtual int camStopExpose (int chip)
  {
    return chips[chip]->stopExposure ();
  }
  virtual int camStopRead (int chip)
  {
    return chips[chip]->endReadout ();
  }
  virtual int camCoolMax ()
  {
    return -1;
  }
  virtual int camCoolHold ()
  {
    return -1;
  }
  virtual int camCoolTemp (float new_temp)
  {
    return -1;
  }
  virtual int camCoolShutdown ()
  {
    return -1;
  }

  // callback functions from camera connection
  int camChipInfo (Rts2Conn * conn, int chip);
  int camExpose (Rts2Conn * conn, int chip, int light, float exptime);
  int camStopExpose (Rts2Conn * conn, int chip);
  int camBox (Rts2Conn * conn, int chip, int x, int y, int width, int height);
  int camCenter (Rts2Conn * conn, int chip, int in_w, int in_h);
  int camReadout (int chip);
  int camReadout (Rts2Conn * conn, int chip);
  // start readout & do/que exposure, that's for frame transfer CCDs
  int camReadoutExpose (Rts2Conn * conn, int chip, int light, float exptime);
  int camBinning (Rts2Conn * conn, int chip, int x_bin, int y_bin);
  int camStopRead (Rts2Conn * conn, int chip);
  int camCoolMax (Rts2Conn * conn);
  int camCoolHold (Rts2Conn * conn);
  int camCoolTemp (Rts2Conn * conn, float new_temp);
  int camCoolShutdown (Rts2Conn * conn);

  virtual int getFilterNum ();

  // focuser functions
  int setFocuser (int new_set);
  int stepFocuser (int step_count);
  int getFocPos ();

  float focusExposure;
  float defFocusExposure;

  // autofocus
  int startFocus (Rts2Conn * conn);
  int endFocusing ();

  double getSubExposure (void)
  {
    return subExposure->getValueDouble ();
  }

  bool isIdle ();
  bool isFocusing ();

  virtual int grantPriority (Rts2Conn * conn)
  {
    if (focuserDevice)
      {
	if (conn->isName (focuserDevice))
	  return 1;
      }
    return Rts2Device::grantPriority (conn);
  }

  int getLastFilterNum ()
  {
    return lastFilterNum;
  }

  virtual int commandAuthorized (Rts2Conn * conn);
};

#endif /* !__RTS2_CAMERA_CPP__ */
