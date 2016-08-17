/* 
 * Abstract rotator class.
 * Copyright (C) 2011 Petr Kubanek <petr@kubanek.net>
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

#include "device.h" 

namespace rts2rotad
{

/**
 * Abstract class for rotators.
 *
 * PA in this class stands for Parallactic Angle.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class Rotator:public rts2core::Device
{
	public:
		/**
		 * Rotator constructor.
		 *
		 * @param ownTimer   if true, rotator own timer to keep trackign will be disabled. Usefully for inteligent devices,
		 *                   which has own tracking, or for multidev where timer is in base device.
		 */
		Rotator (int argc, char **argv, const char *defname = "R0", bool ownTimer = false);

		virtual int commandAuthorized (rts2core::Connection * conn);

		/**
		 * Update tracking frequency. Should be run after new tracking vector
		 * is send to the rotator motion controller.
		 */
		void updateTrackingFrequency ();

		virtual void postEvent (rts2core::Event * event);

		void checkRotators ();

	protected:
		virtual int idle ();

		virtual int setValue (rts2core::Value *old_value, rts2core::Value *new_value);

		/**
		 * Called to set rotator target and start rotating.
		 */
		virtual void setTarget (double tv) = 0;

		void setCurrentPosition (double cp);

		/**
		 * Calculate current PA as linear estimate from the past value and speed.
		 */
		double getPA (double t);

		/**
		 * Runs PA tracking.
		 */
		void runPATracking ();

		/**
		 * Returns >0 if rotator is rotating image.
		 */
		virtual long isRotating () = 0;

		/**
		 * Called at the end of rotation.
		 */
		virtual void endRotation () { infoAll (); }

		double getCurrentPosition () { return currentPosition->getValueDouble (); }
		double getTargetPosition () { return targetPosition->getValueDouble (); }
		double getDifference () { return toGo->getValueDouble (); }

	private:
		rts2core::ValueDouble *currentPosition;
		rts2core::ValueDoubleMinMax *targetPosition;

		rts2core::ValueBool *paTracking;
		rts2core::ValueDoubleStat *trackingFrequency;
		rts2core::ValueInteger *trackingFSize;
		rts2core::ValueFloat *trackingWarning;
		double lastTrackingRun;

		bool hasTimer;

		rts2core::ValueTime *parallacticAngleRef;
		rts2core::ValueDouble *parallacticAngle;
		rts2core::ValueDouble *parallacticAngleRate;
		rts2core::ValueDouble *toGo;

		void updateToGo ();
};

}
