/*!
 * Driver for Apogee camera.
 *
 * Based on original apogge driver.
 *
 * @author petr
 */

#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "CameraIO_Linux.h"

#include "../camera.h"

static CCameraIO       *camera = NULL;

extern int
camera_init (char *device_name, int camera_id)
{
        Camera_Status status;
        if (!camera)
                camera = new CCameraIO ();
        camera->Reset ();
        if (!camera->read_Present())
                return -1;
        status = camera->read_Status ();
        if (status < 0)
                return -1;
        return 0;
}

extern void
camera_done ()
{
        delete camera;
        camera = NULL;
}

extern int
camera_info (struct camera_info *info)
{
        static struct chip_info       ch_info;
        strcpy (info->type, "Apogee ");
        strncat (info->type, camera->m_Sensor, 20);

        strcpy (info->serial_number, "007");
        info->chips = 1;
        info->chip_info = &ch_info;
        
        ch_info.width = camera->m_ImgColumns;
        ch_info.height = camera->m_ImgRows;
        ch_info.binning_horizontal = camera->m_BinX;
        ch_info.binning_vertical = camera->m_BinY;
        ch_info.pixelX = camera->m_PixelXSize;
        ch_info.pixelY = camera->m_PixelYSize;
        ch_info.image_type = CAMERA_SHORT_IMG;
        ch_info.gain = camera->m_Gain;

        info->exposure = 0;
        info->temperature_regulation = camera->read_CoolerStatus ();
        info->temperature_setpoint = camera->read_CoolerSetPoint ();
        info->ccd_temperature = camera->read_Temperature();
        info->air_temperature = nan ("0");
        info->cooling_power = -1;
        info->fan = 0;
        info->filter = camera->m_FilterPosition;
	info->can_df = 1;
        return 0;
}

extern int
camera_fan (int fan_state)
{
  return 0;
}

extern int
camera_expose (int chip, float *exposure, int light)
{
  Camera_Status status;
  camera->Expose (*exposure, light);
  do
  {
    status = camera->read_Status ();
  } while (status == Camera_Status_Exposing);
  return 0;
}

extern int
camera_end_expose (int chip)
{
  camera->Reset ();
  return 0;
}

extern int
camera_binning (int chip_id, int vertical, int horizontal)
{
  camera->m_BinY = vertical;
  camera->m_BinX = horizontal;
  return 0;
}

extern int
camera_readout_line (int chip_id, short start, short length, void *data)
{
  Camera_Status status;
  camera->DigitizeLine ();
  do
  {
     status = camera->read_Status ();
  } while (status == Camera_Status_Downloading);
  camera->GetLine ((unsigned short*) data, length);
  return 0;
}

extern int
camera_dump_line (int chip_id)
{
  camera->Flush (1);
  return 0;
}

extern int
camera_end_readout (int chip_id)
{
  camera->Reset ();
  return 0;
}

extern int
camera_cool_max ()
{
  camera->write_CoolerSetPoint (-100);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

extern int
camera_cool_hold ()
{
  camera->write_CoolerSetPoint (-20);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

extern int
camera_cool_shutdown ()
{
  camera->write_CoolerMode (Camera_CoolerMode_Shutdown);
  return 0;
}

extern int
camera_cool_setpoint (float coolpoint)
{
  camera->write_CoolerSetPoint (coolpoint);
  camera->write_CoolerMode (Camera_CoolerMode_On);
  return 0;
}

extern int
camera_set_filter (int filter)
{
  camera->FilterSet (filter);
  return 0;
}
