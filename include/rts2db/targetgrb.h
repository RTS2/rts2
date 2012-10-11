/*
 * Class for GRB target.
 * Copyright (C) 2005-2007 Petr Kubanek <petr@kubanek.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __RTS2_TARGETGRB__
#define __RTS2_TARGETGRB__

#include "rts2db/target.h"

namespace rts2db
{

class ConstTarget;

class TargetGRB:public ConstTarget
{
	public:
		TargetGRB (int in_tar_id, struct ln_lnlat_posn *in_obs, int in_maxBonusTimeout, int in_dayBonusTimeout, int in_fiveBonusTimeout);
		virtual void load ();
		virtual void getPosition (struct ln_equ_posn *pos, double JD);
		virtual int compareWithTarget (Target * in_target, double grb_sep_limit);
		virtual bool getScript (const char *deviceName, std::string & buf);
		virtual int beforeMove ();
		virtual float getBonus (double JD);
		// some logic needed to distinguish states when GRB position change
		// from last observation. there was update etc..
		virtual int isContinues ();

		/**
		 * Query if target is real GRB, or if was marked by GCN as fake GRB.
		 *
		 * @return True when target is real GRB.
		 */
		bool isGrb () { return grb_is_grb; }

		/**
		 * Returns how many seconds elapsed from grbDate.
		 */
		double getPostSec ();

		/**
		 * Returns GRB date.
		 */
		const double getGrbDate () { return grbDate; }

		/**
		 * Check if target is still valid, e.g. if it does not expires or if it isn't fake.
		 */
		void checkValidity ();

		double getFirstPacket ();
		virtual void printExtra (Rts2InfoValStream & _os, double JD);

		virtual void printDS9Reg (std::ostream & _os, double JD);

		void printGrbList (std::ostream & _os);

		virtual void writeToImage (rts2image::Image * image, double JD);

		virtual double getErrorBox () { return errorbox; }

	protected:
		virtual void getDBScript (const char *camera_name, std::string & script);

	private:
		double grbDate;
		double lastUpdate;
		int gcnPacketType;		 // gcn packet from last update
		int gcnGrbId;
		bool grb_is_grb;
		int shouldUpdate;
		int gcnPacketMin;		 // usefull for searching for packet class
		int gcnPacketMax;

		int maxBonusTimeout;
		int dayBonusTimeout;
		int fiveBonusTimeout;

		struct ln_equ_posn grb;
		double errorbox;
		bool autodisabled;

		const char *getSatelite ();
};

}
#endif							 /* !__RTS2_TARGETGRB__ */
