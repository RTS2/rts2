#ifndef __RTS2_CAMERA_CPP__
#define __RTS2_CAMERA_CPP__

#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

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
  float gain;
  int sendReadoutData (char *data, size_t data_size);
public:
    CameraChip (Rts2DevCamera * in_cam, int in_chip_id);
    CameraChip (Rts2DevCamera * in_cam, int in_chip_id, int in_width,
		int in_height, double in_pixelX, double in_pixelY,
		float in_gain);
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
  virtual int init ()
  {
    return 0;
  }
  int send (Rts2Conn * conn);
  virtual int setBinning (int in_vert, int in_hori)
  {
    binningVertical = in_vert;
    binningHorizontal = in_hori;
    return 0;
  }
  virtual int box (int in_x, int in_y, int in_width, int in_height)
  {
    // tests for -1 -> full size
    if (in_x == -1)
      in_x = chipSize->x;
    if (in_y == -1)
      in_y = chipSize->y;
    if (in_width == -1)
      in_width = chipSize->width;
    if (in_height == -1)
      in_height = chipSize->height;
    if (in_x < chipSize->x || in_y < chipSize->y
	|| ((in_x - chipSize->x) + in_width) > chipSize->width
	|| ((in_y - chipSize->y) + in_height) > chipSize->height)
      return -1;
    chipReadout->x = in_x;
    chipReadout->y = in_y;
    chipReadout->width = in_width;
    chipReadout->height = in_height;
    return 0;
  }
  int center (int in_w, int in_h);
  virtual int startExposure (int light, float exptime)
  {
    return -1;
  }
  int setExposure (float exptime, int in_shutter_state);
  virtual long isExposing ();
  virtual int endExposure ();
  virtual int stopExposure ()
  {
    return endExposure ();
  }
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  void setReadoutConn (Rts2DevConnData * dataConn);
  virtual void deleteConnection (Rts2Conn * conn)
  {
    if (conn == readoutConn)
      {
	readoutConn = NULL;
      }
  }
  virtual int endReadout ();
  void clearReadout ();
  virtual int sendFirstLine ();
  virtual int readoutOneLine ();
  virtual void cancelPriorityOperations ();
};

class Rts2DevCamera:public Rts2Device
{
private:
  char *focuserDevice;
  int lastFilterNum;
protected:
  char *device_file;
  // camera chips
  CameraChip *chips[MAX_CHIPS];
  int chipNum;
  // temperature and others; all in deg Celsius
  float tempAir;
  float tempCCD;
  float tempSet;
  int tempRegulation;
  int coolingPower;
  int fan;

  int canDF;			// if the camera can make dark frames
  char ccdType[64];
  char serialNumber[64];

  Rts2Filter *filter;

  float nightCoolTemp;
protected:
    virtual void cancelPriorityOperations ();
  int defBinning;

public:
    Rts2DevCamera (int argc, char **argv);
    virtual ~ Rts2DevCamera (void);

  virtual int processOption (int in_opt);
  virtual int initChips ();
  virtual Rts2DevConn *createConnection (int in_sock, int conn_num);
  void checkExposures ();
  void checkReadouts ();

  virtual int idle ();

  virtual int deleteConnection (Rts2Conn * conn)
  {
    for (int i = 0; i < chipNum; i++)
      chips[i]->deleteConnection (conn);
    return Rts2Device::deleteConnection (conn);
  }

  virtual int changeMasterState (int new_state);

  virtual int ready ()
  {
    return -1;
  }
  virtual int info ()
  {
    return -1;
  }
  virtual int baseInfo ()
  {
    return -1;
  }

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
  virtual int camReadout (int chip)
  {
    return -1;
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
  virtual int sendInfo (Rts2Conn * conn);
  virtual int sendBaseInfo (Rts2Conn * conn);
  int camChipInfo (Rts2Conn * conn, int chip);
  int camExpose (Rts2Conn * conn, int chip, int light, float exptime);
  int camStopExpose (Rts2Conn * conn, int chip);
  int camBox (Rts2Conn * conn, int chip, int x, int y, int width, int height);
  int camCenter (Rts2Conn * conn, int chip, int in_w, int in_h);
  int camReadout (Rts2Conn * conn, int chip);
  int camBinning (Rts2Conn * conn, int chip, int x_bin, int y_bin);
  int camStopRead (Rts2Conn * conn, int chip);
  int camCoolMax (Rts2Conn * conn);
  int camCoolHold (Rts2Conn * conn);
  int camCoolTemp (Rts2Conn * conn, float new_temp);
  int camCoolShutdown (Rts2Conn * conn);
  int camFilter (Rts2Conn * conn, int new_filter);

  int getFilterNum ()
  {
    if (filter)
      return filter->getFilterNum ();
    return -1;
  }

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
};

class Rts2DevConnCamera:public Rts2DevConn
{
private:
  Rts2DevCamera * master;
  int paramNextChip (int *in_chip);
protected:
    virtual int commandAuthorized ();
public:
    Rts2DevConnCamera (int in_sock, Rts2DevCamera * in_master_device);
};

#endif /* !__RTS2_CAMERA_CPP__ */
