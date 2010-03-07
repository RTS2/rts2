#ifndef __RTS2_DEVCLIPHOT__
#define __RTS2_DEVCLIPHOT__

#include "devscript.h"

using namespace rts2script;

class Rts2DevClientPhotExec:public Rts2DevClientPhot, public DevScript
{
	private:
		// minFlux to be considered as success
		float minFlux;

	protected:
		virtual void unblockWait ()
		{
			Rts2DevClientPhot::unblockWait ();
		}
		virtual void unsetWait ()
		{
			Rts2DevClientPhot::unsetWait ();
		}

		virtual void clearWait ()
		{
			Rts2DevClientPhot::clearWait ();
		}

		virtual int isWaitMove ()
		{
			return Rts2DevClientPhot::isWaitMove ();
		}

		virtual void setWaitMove ()
		{
			Rts2DevClientPhot::setWaitMove ();
		}

		virtual void queCommandFromScript (Rts2Command * com)
		{
			queCommand (com);
		}

		virtual int getFailedCount ()
		{
			return Rts2DevClient::getFailedCount ();
		}

		virtual void clearFailedCount ()
		{
			Rts2DevClient::clearFailedCount ();
		}

		virtual void idle ()
		{
			DevScript::idle ();
			Rts2DevClientPhot::idle ();
		}

		virtual void filterMoveEnd ();

		virtual void integrationStart ();
		virtual void integrationEnd ();
		virtual void addCount (int count, float exp, bool is_ov);

		virtual int getNextCommand ();

	public:
		Rts2DevClientPhotExec (Rts2Conn * in_connection);
		virtual ~ Rts2DevClientPhotExec (void);
		virtual void postEvent (Rts2Event * event);
		virtual void integrationFailed (int status);

		virtual void filterMoveFailed (int status);

		virtual void nextCommand ();
};
#endif							 /* !__RTS2_DEVCLIPHOT__ */
