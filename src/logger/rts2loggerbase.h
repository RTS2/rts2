#include "../utils/rts2devclient.h"
#include "../utils/rts2displayvalue.h"
#include "../utils/rts2command.h"

#define EVENT_SET_LOGFILE RTS2_LOCAL_EVENT+800

/**
 * This class logs given values to given file.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2DevClientLogger:public Rts2DevClient
{
	private:
		std::list < Rts2Value * >logValues;
		std::list < std::string > logNames;

		std::ostream * outputStream;

		time_t nextInfoCall;
		time_t numberSec;

		void fillLogValues ();
	protected:
		void setOutputFile (const char *filename);
	public:
		Rts2DevClientLogger (Rts2Conn * in_conn, double in_numberSec,
			std::list < std::string > &in_logNames);

		virtual ~ Rts2DevClientLogger (void);
		virtual void infoOK ();
		virtual void infoFailed ();

		virtual void idle ();

		// used to set command
		virtual void postEvent (Rts2Event * event);
};

/**
 * Holds informations about which device should log which values at which time interval.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rts2LogValName
{
	public:
		std::string devName;
		double timeout;
		std::list < std::string > valueList;

		Rts2LogValName (std::string in_devName, double in_timeout,
			std::list < std::string > in_valueList)
		{
			devName = in_devName;
			timeout = in_timeout;
			valueList = in_valueList;
		}
};

/**
 * This class is base class for logging.
 */
class Rts2LoggerBase
{
	private:
		std::list < Rts2LogValName > devicesNames;

	protected:
		int readDevices (std::istream & is);

		Rts2LogValName *getLogVal (const char *name);
		int willConnect (Rts2Address * in_addr);

	public:
		Rts2LoggerBase ();
		Rts2DevClient *createOtherType (Rts2Conn * conn, int other_device_type);
};
