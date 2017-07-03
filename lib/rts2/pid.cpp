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

#include "pid.h"

using namespace rts2core;

PID::PID()
{
	kP = 0;
	kI = 0;
	kD = 0;

	reset();
}

void PID::reset()
{
	integral = 0;
	derivative = 0;
	p_error = 0;
}

void PID::setPID(double p, double i, double d)
{
	setP(p);
	setI(i);
	setD(d);
}

double PID::loop(double error, double step)
{
	integral += error;
	derivative = (error - p_error) / step;
	p_error = error;

	return kP * error + kI * integral + kD * derivative;
}
