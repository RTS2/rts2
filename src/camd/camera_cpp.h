#ifndef __RTS2_CAMERA_CPP__
#define __RTS2_CAMERA_CPP__

#include <sys/time.h>
#include <time.h>

#include "../utils/rts2block.h"
#include "../utils/rts2device.h"

#define MAX_CHIPS 2

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
public:
  int chipId;
  struct timeval exposureEnd;
  int readoutLine;
  int sendLine;
  Rts2DevConnData *readoutConn;

  ChipSubset *chipSize;
  ChipSubset *chipReadout;
  int binningVertical;
  int binningHorizontal;
  int pixelX;
  int pixelY;
  float gain;

    CameraChip (int in_chip_id);
    CameraChip (int in_chip_id, int in_width, int in_height, int in_pixelX,
		int in_pixelY, float in_gain);
   ~CameraChip (void);
  int send (Rts2Conn * conn);
  virtual int setBinning (int in_vert, int in_hori)
  {
    binningVertical = in_vert;
    binningHorizontal = in_hori;
    return 0;
  }
  virtual int startExposure (int light, float exptime)
  {
    return -1;
  }
  int setExposure (float exptime);
  virtual long isExposing ();
  int endExposure ();
  virtual int startReadout (Rts2DevConnData * dataConn, Rts2Conn * conn);
  virtual int sendFirstLine ();
  virtual int readoutOneLine ();
};

class Rts2DevCamera:public Rts2Device
{
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
  int filter;
  int canDF;			// if the camera can make dark frames
  char ccdType[64];
  char serialNumber[64];
public:
    Rts2DevCamera (int argc, char **argv);
   ~Rts2DevCamera ();

  int init ();
  int addConnection (int in_sock);
  long checkExposures ();
  int checkReadouts ();
  int idle ();

  // callback functions for Camera alone
  virtual int ready ()
  {
    return -1;
  };
  virtual int info ()
  {
    return -1;
  };
  virtual int baseInfo ()
  {
    return -1;
  };
  virtual int camChipInfo (int chip)
  {
    return -1;
  };
  virtual int camExpose (int chip, int light, float exptime)
  {
    return chips[chip]->startExposure (light, exptime);
  };
  virtual long camWaitExpose (int chip);
  virtual int camStopExpose (int chip)
  {
    return -1;
  };
  virtual int camBox (int chip, int x, int y, int width, int height)
  {
    return -1;
  };
  virtual int camReadout (int chip)
  {
    return -1;
  };
  virtual int camStopRead (int chip)
  {
    return -1;
  };
  virtual int camCoolMax ()
  {
    return -1;
  };
  virtual int camCoolHold ()
  {
    return -1;
  };
  virtual int camCoolTemp (float new_temp)
  {
    return -1;
  };
  virtual int camCoolShutdown ()
  {
    return -1;
  };
  virtual int camFilter (int new_filter)
  {
    return -1;
  };

  // callback functions from camera connection
  int ready (Rts2Conn * conn);
  int info (Rts2Conn * conn);
  int baseInfo (Rts2Conn * conn);
  int camChipInfo (Rts2Conn * conn, int chip);
  int camExpose (Rts2Conn * conn, int chip, int light, float exptime);
  int camStopExpose (Rts2Conn * conn, int chip);
  int camBox (Rts2Conn * conn, int chip, int x, int y, int width, int height);
  int camReadout (Rts2Conn * conn, int chip);
  int camBinning (Rts2Conn * conn, int chip, int x_bin, int y_bin);
  int camStopRead (Rts2Conn * conn, int chip);
  int camCoolMax (Rts2Conn * conn);
  int camCoolHold (Rts2Conn * conn);
  int camCoolTemp (Rts2Conn * conn, float new_temp);
  int camCoolShutdown (Rts2Conn * conn);
  int camFilter (Rts2Conn * conn, int new_filter);
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
