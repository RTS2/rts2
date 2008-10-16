#ifndef __RTS2_NWINDOW__
#define __RTS2_NWINDOW__

#include <curses.h>

#include "rts2nlayout.h"

typedef enum
{ RKEY_ENTER, RKEY_ESC, RKEY_HANDLED, RKEY_NOT_HANDLED }
keyRet;

/**
 * Basic class for NCurser windows.
 */
class Rts2NWindow:public Rts2NLayout
{
	private:
		bool _haveBox;
		bool active;
	protected:
		WINDOW * window;
		void errorMove (const char *op, int y, int x, int h, int w);
	public:
		Rts2NWindow (int x, int y, int w, int h, bool border = true);
		virtual ~ Rts2NWindow (void);
		/**
		 * Handles key pressed event. Return values has following meaning:
		 * RKEY_ENTER - enter key was pressed, we should do some action and possibly exit the window
		 * RKEY_ESC - EXIT or ESC key was pressed, we should exit the window
		 * RKEY_HANDLED - key was handled by the window, futher processing should not happen
		 * RKEY_NOT_HANDLED - key was not handled, futher processing/default fall-back call is necessary
		 */
		virtual keyRet injectKey (int key);
		virtual void draw ();

		int getX ();
		int getY ();
		int getCurX ();
		int getCurY ();
		int getWidth ();
		int getHeight ();
		int getWriteWidth ();
		int getWriteHeight ();

		void setX (int x)
		{
			move (x, getY ());
		}

		void setY (int y)
		{
			move (getX (), y);
		}

		virtual void clear ()
		{
			werase (window);
		}

		void move (int x, int y);
		virtual void resize (int x, int y, int w, int h);
		void grow (int max_w, int h_dif);

		virtual void refresh ();

		/**
		 * Gets called when window get focus.
		 */
		virtual void enter ();

		/**
		 * Gets called when window lost focus.
		 */
		virtual void leave ();

		/**
		 * Set screen cursor to current window.
		 * Return true if the window claims cursor, otherwise return false.
		 */
		virtual bool setCursor ();

		void setNormal ()
		{
			wchgat (getWriteWindow (), getWriteWidth (), A_NORMAL, 0, NULL);
		}

		/**
		 * Set reverse attribute for full pad (e.g. invert colors in pad).
		 */
		void setReverse ()
		{
			wchgat (getWriteWindow (), getWriteWidth (), A_REVERSE, 0, NULL);
		}

		void setUnderline ()
		{
			wchgat (getWriteWindow (), getWriteWidth (), A_UNDERLINE, 0, NULL);
		}

		/**
		 * Returns window which is used to write text
		 */
		virtual WINDOW *getWriteWindow ()
		{
			return window;
		}

		bool haveBox ()
		{
			return _haveBox;
		}

		/**
		 * Indicate this window needs enter, so enter and tab keys will not be stolen for wait command.
		 */
		virtual bool hasEditBox ()
		{
			return false;
		}

		/**
		 * Return window active state. That is true if window is active and receives keyboard inputs.
		 *
		 * @return window active state.
		 */
		bool isActive ()
		{
			return active;
		}
};
#endif							 /* !__RTS2_NWINDOW__ */
