#include "rts2execclidb.h"
#include "../writers/rts2imagedb.h"

Rts2DevClientCameraExecDb::Rts2DevClientCameraExecDb (Rts2Conn * in_connection):Rts2DevClientCameraExec
(in_connection)
{
}


Rts2DevClientCameraExecDb::~Rts2DevClientCameraExecDb (void)
{

}


void
Rts2DevClientCameraExecDb::exposureStarted ()
{
	if (currentTarget)
		currentTarget->startObservation (getMaster ());
	Rts2DevClientCameraExec::exposureStarted ();
}


Rts2Image *
Rts2DevClientCameraExecDb::createImage (const struct timeval *expStart)
{
	imgCount++;
	exposureScript = getScript ();
	if (currentTarget)
		// create image based on target type and shutter state
		return new Rts2ImageDb (currentTarget, this, expStart);
	logStream (MESSAGE_ERROR)
		<< "Rts2DevClientCameraExec::createImage creating no-target image"
		<< sendLog;
	return NULL;
}


void
Rts2DevClientCameraExecDb::beforeProcess (Rts2Image * image)
{
	Rts2Image *outimg = setImage (image, setValueImageType (image));
	img_type_t imageType = outimg->getImageType ();
	// dark images don't need to wait till imgprocess will pick them up for reprocessing
	if (imageType == IMGTYPE_DARK)
	{
		outimg->toDark ();
	}
	else if (imageType == IMGTYPE_FLAT)
	{
		outimg->toFlat ();
	}
}
