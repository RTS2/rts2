#include "cupola.h"

/*!
 * Dummy copula for testing.
 */
class Rts2DevCupolaDummy:public Rts2DevCupola
{
	private:
		Rts2ValueInteger * mcount;
		Rts2ValueInteger *moveCountTop;
	protected:
		virtual int moveStart ()
		{
			mcount->setValueInteger (0);
			return Rts2DevCupola::moveStart ();
		}
		virtual int moveEnd ()
		{
			struct ln_hrz_posn hrz;
			getTargetAltAz (&hrz);
			setCurrentAz (hrz.az);
			return Rts2DevCupola::moveEnd ();
		}
		virtual long isMoving ()
		{
			if (mcount->getValueInteger () >= moveCountTop->getValueInteger ())
				return -2;
			mcount->inc ();
			return USEC_SEC;
		}
		virtual int setValue (Rts2Value * old_value, Rts2Value * new_value)
		{
			if (old_value == moveCountTop)
			{
				return 0;
			}
			return Rts2DevCupola::setValue (old_value, new_value);
		}
	public:
		Rts2DevCupolaDummy (int in_argc, char **in_argv):Rts2DevCupola (in_argc,
			in_argv)
		{
			createValue (mcount, "mcount", "moving count", false);
			createValue (moveCountTop, "moveCountTop", "move count top", false);
			moveCountTop->setValueInteger (100);
		}

		virtual int initValues ()
		{
			setCurrentAz (0);
			return Rts2DevCupola::initValues ();
		}

		virtual int ready ()
		{
			return 0;
		}

		virtual int baseInfo ()
		{
			return 0;
		}

		virtual int openDome ()
		{
			mcount->setValueInteger (0);
			return Rts2DevCupola::openDome ();
		}

		virtual long isOpened ()
		{
			return isMoving ();
		}

		virtual int closeDome ()
		{
			mcount->setValueInteger (0);
			return Rts2DevCupola::closeDome ();
		}

		virtual long isClosed ()
		{
			return isMoving ();
		}

		virtual double getSplitWidth (double alt)
		{
			return 1;
		}
};

int
main (int argc, char **argv)
{
	Rts2DevCupolaDummy device = Rts2DevCupolaDummy (argc, argv);
	return device.run ();
}
