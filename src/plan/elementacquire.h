#ifndef __RTS2_SCRIPT_ELEMENT_ACQUIRE__
#define __RTS2_SCRIPT_ELEMENT_ACQUIRE__

#include "script.h"
#include "element.h"

namespace rts2script
{

class ElementAcquire:public Element
{
	public:
		ElementAcquire (Script * in_script, double in_precision, float in_expTime, struct ln_equ_posn *in_center_pos);
		virtual void postEvent (Rts2Event * event);
		virtual int nextCommand (Rts2DevClientCamera * camera, Rts2Command ** new_command, char new_device[DEVICE_NAME_SIZE]);
		virtual int processImage (rts2image::Image * image);
		virtual void cancelCommands ();

		virtual void printScript (std::ostream &os) { os << COMMAND_ACQUIRE << " " << reqPrecision << " " << expTime; }
	protected:
		double reqPrecision;
		double lastPrecision;
		float expTime;

		enum { NEED_IMAGE, WAITING_IMAGE, WAITING_ASTROMETRY, WAITING_MOVE, PRECISION_OK, PRECISION_BAD, FAILED} processingState;

		std::string defaultImgProccess;
		int obsId;
		int imgId;
		struct ln_equ_posn center_pos;
};

/**
 * Class for bright source acquistion
 */
class ElementAcquireStar:public ElementAcquire
{
	public:
		/**
		 * @param in_spiral_scale_ra  RA scale in degrees
		 * @param in_spiral_scale_dec DEC scale in degrees
		 */
		ElementAcquireStar (Script * in_script, int in_maxRetries, double in_precision, float in_expTime, double in_spiral_scale_ra, double in_spiral_scale_dec, struct ln_equ_posn *in_center_pos);
		virtual ~ ElementAcquireStar (void);
		virtual void postEvent (Rts2Event * event);
		virtual int processImage (rts2image::Image * image);

		virtual void printScript (std::ostream &os) { os << COMMAND_STAR_SEARCH << " " << maxRetries << " " << minFlux; }
	protected:
		/**
		 * Decide, if image contains source of interest...
		 *
		 * It's called from processImage to decide what to do.
		 *
		 * @return -1 when we should continue in spiral search, 0 when source is in expected position,
		 * 1 when source is in field, but offset was measured; in that case it fills ra_offset and dec_offset.
		 */
		virtual int getSource (rts2image::Image * image, double &ra_off, double &dec_off);
	private:
		int maxRetries;
		int retries;
		double spiral_scale_ra;
		double spiral_scale_dec;
		double minFlux;
		Rts2Spiral *spiral;
};

/**
 * Some special handling for HAM..
 */
class ElementAcquireHam:public ElementAcquireStar
{
	public:
		ElementAcquireHam (Script * in_script, int in_maxRetries, float in_expTime, struct ln_equ_posn *in_center_pos);
		virtual ~ ElementAcquireHam (void);

		virtual void printScript (std::ostream &os) { os << COMMAND_HAM << " " << maxRetries; }
	protected:
		virtual int getSource (rts2image::Image * image, double &ra_off, double &dec_off);
	private:
		int maxRetries;
		int retries;
};

}
#endif							 /* !__RTS2_SCRIPT_ELEMENT_ACQUIRE__ */
