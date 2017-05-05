/* 
 * Simple PID controller.
 * Copyright (C) 2017 Petr Kubanek <petr@kubanek.net>
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
 */
#ifndef __RTS2_PID__
#define __RTS2_PID__

namespace rts2core
{

/**
 * Implements PID (Proportional - Integration - Derivative) control element.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class PID
{
	public:
		PID();
		void reset();

		void setP(double p) { kP = p; }
		void setI(double i) { kI = i; }
		void setD(double d) { kD = d; }
		void setPID(double p, double i, double d);

		double getP() { return kP; }
		double getI() { return kI; }
		double getD() { return kD; }

		/**
		 * Performs PID iteration. Records error on the go.
		 */
		double loop (double error, double step = 1);

	private:
		double kP;
		double kI;
		double kD;
	
		double integral;
		double derivative;
		double p_error;
};

}

#endif // !__RTS2_PID__
