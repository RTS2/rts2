/* 
 * Altitude plotting library.
 * Copyright (C) 2010 Petr Kubanek <petr@kubanek.net>
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

#include <config.h>

#ifdef HAVE_LIBJPEG

#include <Magick++.h>
#include "../utilsdb/records.h"
#include "plot.h"

namespace rts2xmlrpc
{

/**
 * Value graph class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValuePlot:public Plot
{
	public:
		/**
		 * Create plot for a given variable.
		 *
		 * @param _varId    Varible which will be plotted.
		 * @param _valType  Display type of variable
		 */
		ValuePlot (int _varId, int _valType, int w = 800, int h = 600);

		/**
		 * Return Magick::Image plot of the data.
		 *
		 * @param from       Plot from this time.
		 * @param to         Plot to this time.
		 * @param plotType   Type of value plot.
		 * 
		 * @throw rts2core::Error or its descendandts on error.
		 */
		Magick::Image* getPlot (double _from, double _to, Magick::Image* _image = NULL, PlotType _plotType = PLOTTYPE_AUTO, int linewidth = 3, int shadow = 5);
	
	private:
		int recvalId;
		int valueType;
};

}

#endif /* HAVE_LIBJPEG */
