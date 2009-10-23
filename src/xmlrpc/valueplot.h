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

#include <Magick++.h>
#include "../utilsdb/records.h"

namespace rts2xmlrpc
{

typedef enum {PLOTTYPE_AUTO, PLOTTYPE_LINE, PLOTTYPE_CROSS, PLOTTYPE_CIRCLES, PLOTTYPE_SQUARES, PLOTTYPE_FILL, PLOTTYPE_FILL_SHARP} PlotType;

/**
 * Generates graph for variable.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 */
class ValuePlot
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
		Magick::Image* getPlot (double _from, double _to, Magick::Image* _image = NULL, PlotType _plotType = PLOTTYPE_AUTO, int shadow = 5);
	
	private:
		int recvalId;
		int valueType;

		double scaleX;
		double scaleY;

		double min;
		double max;

		double from;
		double to;

		PlotType plotType;

		Magick::Geometry size;
		Magick::Image *image;

		void plotData (rts2db::RecordsSet &rs, Magick::Color col, int shadow);

		void plotYGrid (int y);
		void plotXGrid (int x);

		void plotYDegrees ();
		void plotYDouble ();
		void plotYBoolean ();
};

}
