#ifndef __RTS2_SWAITFOR__
#define __RTS2_WAITFOR__

#include "rts2script.h"

/**
 * Used for waitfor command. Idle call returns NEXT_COMMAND_KEEP when target
 * value is not reached, and NEXT_COMMAND_NEXT when target value is reached.
 */
class Rts2SWaitFor:public Rts2ScriptElement
{
	private:
		std::string deviceName;
		std::string valueName;
		double tarval;
		double range;
	protected:
		virtual void getDevice (char new_device[DEVICE_NAME_SIZE]);
	public:
		Rts2SWaitFor (Rts2Script * in_script, const char *new_device,
			char *valueName, double value, double range);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int idle ();
};

/**
 * Used for sleep command. Return NEXT_COMMAND_KEEP in idle call while command
 * should be keeped, and NEXT_COMMAND_NEXT when timeout expires.
 */
class Rts2SSleep:public Rts2ScriptElement
{
	private:
		double sec;
	public:
		Rts2SSleep (Rts2Script * in_script, double in_sec);
		virtual int defnextCommand (Rts2DevClient * client,
			Rts2Command ** new_command,
			char new_device[DEVICE_NAME_SIZE]);
		virtual int idle ();
};
#endif							 /* !__RTS2_WAITFOR__ */
