#include "camera_info.h"
#include "../camera.h"

#include "lsbigusbcam.h"
#include "lsbiglptcam.h"

#include <stdio.h>
#include <string.h>
#include <syslog.h>

SbigCam		*pcam = (SbigCam*)NULL;


int camera_init (char *device_name, int camera_id)
{
	if (!pcam) 
	{
		pcam = new SbigUsbCam("sbigusb0");
		if (pcam->GetStatus() == CE_NO_ERROR)
		{
			pcam->EstablishLink();
			if (pcam->GetStatus() != CE_NO_ERROR)
			{
				syslog (LOG_ERR, "Errror at Establish link: %s", pcam->GetStatusString ());
				delete pcam;
				pcam = (SbigCam*)NULL;
				return -1;
			}
			return 0;
		}
	}
	return -1;
}

void camera_done ()
{
	delete pcam;
	pcam = (SbigCam*)NULL;
}

int camera_info (struct camera_info *info)
{
	GetCCDInfoParams	gccdip;
	GetCCDInfoResults0	gccdir0;
	if (!pcam)
		return -1;
	gccdip.request = 0;
	pcam->GetCCDInfo (&gccdip, (void *)&gccdir0);
	if (pcam->GetStatus () != CE_NO_ERROR)
		goto err;
	sprintf (info->type, "%s %i", pcam->GetDeviceName(), pcam->GetCameraType());
	SetTemperatureRegulationParams	strp;
	QueryTemperatureStatusResults	qtsr;
	if (pcam->QueryTemperatureStatus(&qtsr) != CE_NO_ERROR)
		goto err;
	info->temperature_regulation = 1;
//	info->temperature_setpoint = pcam->Calc
	info->air_temperature = pcam->CalcAmbTemperature (&qtsr);
	info->ccd_temperature = pcam->CalcCcdTemperature (&qtsr);
	return 0;
err:
	delete pcam;
	pcam = (SbigCam*) NULL;
	return -1;
}

int camera_fan (int fan_state)
{
	return 0;
}

int camera_expose (int chip, float *exposure, int light)
{
	return 0;
}

int camera_end_expose (int chip)
{
	return 0;
}

int camera_binning (int chip_id, int vertical, int horizontal)
{
	return 0;
}

int camera_readout_line (int chip_id, short start, short length,
				void *data)
{
	return 0;
}

int camera_dump_line (int chip_id)
{
	return 0;
}

int camera_end_readout (int chip_id)
{
	return 0;
}

int camera_cool_max ()
{
	return 0;
}

int camera_cool_hold ()
{
	return 0;
}

int camera_cool_shutdown ()
{
	return 0;
}

int camera_cool_setpoint (float coolpoint)
{
	return 0;
}
