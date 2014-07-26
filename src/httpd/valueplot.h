/* 
 * Variable plotting library.
 * Copyright (C) 2009 Petr Kubanek <petr@kubanek.net>
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

#include <rts2-config.h>

#ifdef RTS2_HAVE_LIBJPEG

#include <Magick++.h>
#include "rts2db/records.h"
#include "rts2json/plot.h"

namespace rts2xmlrpc
{

/**
 * Value graph class.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValuePlot:public rts2json::Plot
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
		 * @param plotSun    Plot sun altitude (dark/white background)
		 * @param plotShadow Plot Sun shadow (night/day, twilight)
		 * @param localDate  Use local date
		 * 
		 * @throw rts2core::Error or its descendandts on error.
		 */
		Magick::Image* getPlot (double _from, double _to, Magick::Image* _image = NULL, rts2json::PlotType _plotType = rts2json::PLOTTYPE_AUTO, int linewidth = 3, int shadow = 5, bool plotSun = true, bool plotShadow = true, bool localDate = true);
	
	private:
		int recvalId;
		int valueType;

		void plotData (rts2db::RecordsSet &rs, Magick::Color col, int linewidth, int shadow);
};

}

#endif /* RTS2_HAVE_LIBJPEG */
