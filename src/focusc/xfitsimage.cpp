/* 
 * Display RTS2 (and FITS images) in separate window.
 * Copyright (C) 2012 Petr Kubanek, Institute of Physics <kubanek@fzu.cz>
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

#include "xfitsimage.h"
#include "command.h"

#include <rts2-config.h>

#ifdef RTS2_HAVE_CURSES_H
#include <curses.h>
#elif defined(RTS2_HAVE_NCURSES_CURSES_H)
#include <ncurses/curses.h>
#endif

#include <iomanip>

int cmpbyte (const void *a, const void *b)
{
#define A (*((uint8_t*) a))
#define B (*((uint8_t*) b))
	return A > B ? 1 : -1;
#undef A
#undef B
}

int cmpuint16_t (const void *a, const void *b)
{
#define A (*((uint16_t*) a))
#define B (*((uint16_t*) b))
	return A > B ? 1 : -1;
#undef A
#undef B
}

XFitsImage::XFitsImage (rts2core::Connection *_connection, rts2core::DevClient *_client)
{
	connection = _connection;
	client = _client;

	window = 0L;
	pixmap = 0L;
	ximage = NULL;
	gc = 0L;
	windowHeight = 800;
	windowWidth = 800;

	pixmapHeight = windowHeight;
	pixmapWidth = windowWidth;

	mouseX = mouseY = -1;
	buttonX = buttonY = -1;

	timerclear (&buttonImageTime);
	timerclear (&exposureStart);

	mouseTelChange_x = INT_MAX;
	mouseTelChange_y = INT_MAX;

	lastImage = false;

	low = high = min = max = 0;
	median = average = 0;
	binningsX = binningsY = 0;
}

XFitsImage::~XFitsImage ()
{
	if (ximage)
		XDestroyImage (ximage);
	if (gc)
		XFreeGC (display, gc);
}

void XFitsImage::XeventLoop ()
{
	XEvent event;
	KeySym ks;
//	struct ln_equ_posn change;

	if (XCheckWindowEvent (display, window, KeyPressMask | ButtonPressMask | ExposureMask | PointerMotionMask, &event))
	{
		switch (event.type)
		{
			case Expose:
				if (pixmap && gc)
					redraw ();
				break;
			case KeyPress:
				ks = XLookupKeysym ((XKeyEvent *) & event, 0);
				switch (ks)
				{
					case XK_1:
						connection->queCommand (new rts2core::CommandChangeValue (client, "binning", '=', 0));
						break;
					case XK_2:
						connection->queCommand (new rts2core::CommandChangeValue (client, "binning", '=', 1));
						break;
					case XK_3:
						connection->queCommand (new rts2core::CommandChangeValue (client, "binning", '=', 2));
						break;
	/*				case XK_9:
						connection->GoNine = !master->GoNine;
						break; */
					case XK_e:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '+', 1));
						break;
					case XK_d:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '-', 1));
						break;
					case XK_w:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '+', 0.1));
						break;
					case XK_s:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '-', 0.1));
						break;
					case XK_q:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '+', 0.01));
						break;
					case XK_a:
						connection->queCommand (new rts2core::CommandChangeValue (client, "exposure", '-', 0.01));
						break;
/*					case XK_f:
						connection->queCommand (new rts2core::Command (connection, "box 0 -1 -1 -1 -1"));
						break;
					case XK_c:
						connection->queCommand (new rts2core::Command (client, "center 0"));
						break;
					case XK_p:
						connection->postEvent (new rts2core::Event (EVENT_INTEGRATE_START));
						break;
					case XK_o:
						connection->postEvent (new rts2core::Event (EVENT_INTEGRATE_STOP));
						break;
					case XK_y:
						setSaveImage (1);
						break;
					case XK_u:
						setSaveImage (0 || exe);
						break;
						// change stuff
					case XK_h:
					case XK_Left:
						change.ra = -1 * change_val;
						change.dec = 0;
						master->postEvent (new rts2core::Event (EVENT_MOUNT_CHANGE, (void *) &change));
						break;
					case XK_j:
					case XK_Down:
						change.ra = 0;
						change.dec = -1 * change_val;
						master->postEvent (new rts2core::Event (EVENT_MOUNT_CHANGE, (void *) &change));
						break;
					case XK_k:
					case XK_Up:
						change.ra = 0;
						change.dec = change_val;
						master->postEvent (new rts2core::Event (EVENT_MOUNT_CHANGE, (void *) &change));
						break;
					case XK_l:
					case XK_Right:
						change.ra = change_val;
						change.dec = 0;
						master->postEvent (new rts2core::Event (EVENT_MOUNT_CHANGE, (void *) &change));
						break;
					case XK_Escape:
						master->endRunLoop ();
						break;
					case XK_plus:
					case XK_KP_Add:
						master->zoom = master->zoom * 2.0;
						break;
					case XK_minus:
					case XK_KP_Subtract:
						master->zoom = master->zoom / 2.0;
						break; */
					default:
						break;
				}
				break;
			case MotionNotify:
				mouseX = ((XMotionEvent *) & event)->x;
				mouseY = ((XMotionEvent *) & event)->y;

				printMouse ();
				redrawMouse ();
				XFlush (display);
				break;
			case ButtonPress:
				mouseX = ((XButtonPressedEvent *) & event)->x;
				mouseY = ((XButtonPressedEvent *) & event)->y;
				if (((XButtonPressedEvent *) & event)->state & ControlMask)
				{
					if (buttonX < 0 && buttonY < 0)
					{
						buttonX = mouseX;
						buttonY = mouseY;
						buttonImageTime = exposureStart;
					}
					else
					{
						// calculate distance travelled, print it, pixels / sec travelled, discard button
						timersub (&exposureStart, &buttonImageTime, &exposureStart);
						double del = exposureStart.tv_sec + (double) exposureStart.tv_usec / USEC_SEC;
						double offsetX = buttonX - mouseX;
						double offsetY = buttonY - mouseY;
						if (del > 0)
						{
							printf ("Delay %.4f sec\nX offset: %.1f drift: %.1f pixels/sec\nY offset: %.1f drift: %1.f pixels/sec\n", del, offsetX, offsetX / del, offsetY, offsetY / del);
						}
						else
						{
							printf ("Delay %.4f sec\nX offset: %.1f\nY offset: %.1f\n", del, offsetX, offsetY);
						}
						// clear results
						buttonX = -1;
						buttonY = -1;
						timerclear (&buttonImageTime);
					}
				}
				else if (((XButtonPressedEvent *) & event)->button == Button1)
				{
					// move by given direction
					mouseTelChange_x = mouseX;
					mouseTelChange_y = mouseY;
					std::cout << "Change" << std::endl;
				}
				else
				{
					markers.push_back (Marker (mouseX, mouseY));
				}

				printf ("%i %i\n", mouseX, mouseY);

				printMouse ();
				redrawMouse ();
				break;
		}
	}
}

void XFitsImage::drawImage (rts2image::Image * image, int chan, Display * _display, Visual * _visual, int _depth, double zoom, int _crossType, bool GoNine)
{
	display = _display;
	visual = _visual;
	depth = _depth;

	average = image->getAverage ();

	channelnum = image->getChannelNumber (chan);

	crossType = _crossType;

	int i, j;

	if (window == 0L)
		buildWindow ();

	if (ximage && (pixmapWidth < image->getChannelWidth (chan) || pixmapHeight < image->getChannelHeight (chan)))
	{
		XDestroyImage (ximage);
		ximage = NULL;
	}

	if (GoNine)
	{
		XWindowAttributes wa;
		XGetWindowAttributes (display, DefaultRootWindow (display), &wa);
		pixmapWidth = wa.width / 2;
		pixmapHeight = wa.height / 2;

		// if window is too large, set to 1/4 of required size, so we will see nine effect
		if (pixmapWidth > image->getChannelWidth (chan) * zoom)
			pixmapWidth = (int) ceil (image->getChannelWidth (chan) * zoom);
		if (pixmapHeight > image->getChannelHeight (chan) * zoom)
		  	pixmapHeight = (int) ceil (image->getChannelHeight (chan) * zoom);
	}
	else
	{
		pixmapWidth = (int) ceil (image->getChannelWidth (chan) * zoom);
		pixmapHeight = (int) ceil (image->getChannelHeight (chan) * zoom);
	}

	if (pixmapWidth != windowWidth || pixmapHeight != windowHeight)
	{
		windowWidth = pixmapWidth;
		windowHeight = pixmapHeight;
		rebuildWindow ();
	}
	// draw window with image..
	if (!ximage)
	{
		logStream (MESSAGE_DEBUG) << "creating ximage " << pixmapWidth << "x" << pixmapHeight << sendLog;
		ximage = XCreateImage (display, visual, depth, ZPixmap, 0, 0, pixmapWidth, pixmapHeight, 8, 0);
		ximage->data = (char *) malloc (ximage->bytes_per_line * pixmapHeight);
	}

	// default vertical and horizontal image origins - center image
	int vorigin = (int) floor (zoom * (double) image->getChannelWidth (chan) / 2.0) - pixmapWidth / 2;
	int horigin = (int) floor (zoom * (double) image->getChannelHeight (chan) / 2.0) - pixmapHeight / 2;

	// create array which will hold the image
	// this will be then zoomed to pixmap array
	// to make it clear: iP is temporaly image storage, used to extract
	// data from image and calculate image cut levels, ximage->data then
	// holds pixmap, which is zoomed
	// modified image size
	int iW = (int) ceil (pixmapWidth / zoom);
	int iH = (int) ceil (pixmapHeight / zoom);
	int iPixelByteSize = image->getPixelByteSize ();
	char *iP = (char *) malloc (iW * iH * iPixelByteSize);
	char *iTop = iP;
	// pointer to top line of square image subset
	const char *iNineTop = (const char *) image->getChannelData (chan);
	// prepare the image data to be processed
	const char *im_ptr = (const char *) image->getChannelData (chan) + iPixelByteSize * (vorigin + horigin * image->getChannelWidth (chan));

	// fill IP
	// copy image center..
	for (i = 0; i < iH / 3; i++)
	{
		if (GoNine)
		{
		 	// square..
			memcpy (iTop, iNineTop, iPixelByteSize * (iW / 3));
			iTop += iPixelByteSize * (iW / 3);

			// line..
			// im_ptr is offseted in horizontal direction, so we only add iW/3 for already copied pixels..
			memcpy (iTop, im_ptr + iPixelByteSize * (iW / 3), iPixelByteSize * (int) ceil ((double) iW / 3.0));
			iTop += iPixelByteSize * ((int) ceil ((double) iW / 3.0));

			memcpy (iTop, iNineTop + iPixelByteSize * (image->getChannelWidth (chan) - 2 * iW / 3), iPixelByteSize * (iW / 3));
			iTop += iPixelByteSize * (iW / 3);

			iNineTop += iPixelByteSize * image->getChannelWidth (chan);
		}
		else
		{
			memcpy (iTop, im_ptr, iPixelByteSize * iW);
			iTop += iPixelByteSize * iW;
		}
		im_ptr += iPixelByteSize * image->getChannelWidth (chan);
	}
	// only center in all cases..
	for (;i < 2 * iH / 3; i++)
	{
		memcpy (iTop, im_ptr, iPixelByteSize * iW);
		im_ptr += iPixelByteSize * image->getChannelWidth (chan);
		iTop += iPixelByteSize * iW;
	}

	if (GoNine)
		iNineTop += iPixelByteSize * (image->getChannelWidth (chan) * (image->getChannelHeight (chan) - (int) floor (2 * iH / 3.0)));

	// followed again by edge squares for end..
	for (;i < iH; i++)
	{
		if (GoNine)
		{
		 	// square..
			memcpy (iTop, iNineTop, iPixelByteSize * (iW / 3));
			iTop += iPixelByteSize * (iW / 3);

			// line..
			memcpy (iTop, im_ptr + 2 * iW / 3, iPixelByteSize * ((int) ceil ((double) iW / 3.0)));
			iTop += iPixelByteSize * ((int) ceil ((double) iW / 3.0));

			memcpy (iTop, iNineTop + (image->getChannelWidth (chan) - 2 * iW / 3), iPixelByteSize * (iW / 3));
			iTop += iPixelByteSize  * (iW / 3);
			iNineTop += iPixelByteSize * image->getChannelWidth (chan);
		}
		else
		{
			memcpy (iTop, im_ptr, iPixelByteSize * iW);
			iTop += iPixelByteSize * iW;
		}
		im_ptr += iPixelByteSize * image->getChannelWidth (chan);
	}

	// get cuts
	double sigma;
	median = classical_median (iP, image->getDataType (), iW * iH, &sigma);
	low = (int) (median - 3 * sigma);
	high = (int) (median + 5 * sigma);

	// clear progress indicator
	std::cout << std::setw (COLS) << " " << '\r';

	if (image->getChannelSize () > 1)
	{
		std:: cout << Timestamp () << " multiple channel image" << std::endl;
		for (int ch = 0; ch < image->getChannelSize (); ch++)
		{
			std::cout << "channel " << (ch + 1) << " image [" << image->getChannelWidth (ch) << "x" << image->getChannelHeight (ch) << "x" << image->getPixelByteSize () << "]" << std::endl;
		}
	}
	else
	{
		std::cout << Timestamp () << " single channel image [" << image->getChannelWidth (chan) << "x" << image->getChannelHeight (chan) << "x" << image->getPixelByteSize () << "]" << std::endl;
	}
	std::cout << "Window median:" << median << " sigma " << sigma << " low:" << low << " high:" << high << std::endl;
	// transfer iP to pixmap, zoom it on fly
	for (i = 0; i < pixmapHeight; i++)
	{
		for (j = 0; j < pixmapWidth; j++)
		{
			double pW; // pixel value
			// do zooming..
			if (zoom >= 1)
			{
				switch (image->getDataType ())
				{
					case RTS2_DATA_BYTE:
						pW = (double) ((uint8_t *) iP)[((int) (i / zoom)) * iW + (int) (j / zoom)];
						break;
					case RTS2_DATA_USHORT:
						pW = (double) ((uint16_t *) iP)[((int) (i / zoom)) * iW + (int) (j / zoom)];
						break;
				}
			}
			else
			{
				pW = 0;
				// collect relevant pixels..
				int bsize = (int) ceil (1 / zoom);

				int ii = (int) floor (i * bsize);
				int jj = (int) floor (j * bsize);

				for (int k = 0; k < bsize; k++)
					for (int l = 0; l < bsize; l++)
					{
						switch (image->getDataType ())
						{
							case RTS2_DATA_BYTE:
								pW += ((uint8_t *) iP)[(ii + k) * iW + jj + l];
								break;
							case RTS2_DATA_USHORT:
								pW += ((uint16_t *) iP)[(ii + k) * iW + jj + l];
								break;
						}
					}

				pW /= bsize * bsize;

			}
			int grey = (int) floor (256.0 * (pW - (double) low) / ((double) high - (double) low));

			if (grey < 0) grey=0;
			if (grey > 255) grey=255;

			// here the pixel position is clear
			XPutPixel (ximage, j, i, rgb[grey].pixel);
		}
	}

	free (iP);

	XResizeWindow (display, window, pixmapWidth, pixmapHeight);

	XPutImage (display, pixmap, gc, ximage, 0, 0, 0, 0, pixmapWidth, pixmapHeight);

	// some info values
	image->getValue ("X", lastX);
	image->getValue ("Y", lastY);
	lastSizeX = image->getChannelWidth (chan);
	lastSizeY = image->getChannelHeight (chan);
	image->getValue ("BINX", binningsX);
	image->getValue ("BINY", binningsY);

	exposureStart.tv_sec = image->getExposureSec ();
	exposureStart.tv_usec = image->getExposureUsec ();

	if (!lastImage)
		lastImage = true;

	// process mouse change events..

	/*if (mouseTelChange_x != INT_MAX && mouseTelChange_y != INT_MAX && getActualImage ())
	{
		//struct ln_equ_posn change;
		//getActualImage ()->getRaDec (mouseTelChange_x, mouseTelChange_y, change.ra, change.dec);

		//change.ra -= getActualImage ()->getCenterRa ();
		//change.dec -= getActualImage ()->getCenterDec ();

		//logStream (MESSAGE_DEBUG) << "Change X:" << mouseTelChange_x << " Y:" << mouseTelChange_y << " RA:" << change.ra << " DEC:" << change.dec << sendLog;
		//master->postEvent (new rts2core::Event (EVENT_MOUNT_CHANGE, (void *) &change));
		//mouseTelChange_x = INT_MAX;
		//mouseTelChange_y = INT_MAX;
	}*/

	redraw ();
	XFlush (display);
}

double XFitsImage::classical_median (void *q, int16_t dataType, int n, double *sigma, double sf)
{
	int i;
	double M,S;
	char *f;

	switch (dataType)
	{
		case RTS2_DATA_BYTE:
			f = (char *) new uint8_t[n];
			memcpy (f, q, n * sizeof (uint8_t));
			qsort (f, n, sizeof (uint8_t), cmpbyte);
#define F ((char *) f)
			if (n % 2)
				M = F[n / 2];
			else
		 	 	M = (F[n / 2] + F[n / 2 + 1]) / 2;
			min = F[0];
			max = F[n - 1];
#undef F
			break;
		case RTS2_DATA_USHORT:
			f = (char *) new uint16_t[n];
			memcpy (f, q, n * sizeof (uint16_t));
			qsort (f, n, sizeof (uint16_t), cmpuint16_t);
#define F ((uint16_t *) f)
			if (n % 2)
				M = F[n / 2];
			else
		 	 	M = (F[n / 2] + F[n / 2 + 1]) / 2;
			min = F[0];
			max = F[n - 1];	
#undef F
			break;
		default:
			throw rts2core::Error ("unsuported data type");
	}

	if (sigma)
	{
		switch (dataType)
		{
			case RTS2_DATA_BYTE:
#define F ((uint8_t *) f)
				for (i = 0; i < n; i++)
					F[i] = (uint8_t) fabs(F[i] - M);
				qsort (f, n, sizeof(uint8_t), cmpbyte);
				if (n % 2)
					S = F[n / 2];
				else
					S = (F[n / 2] + F[n / 2 + 1]) / 2;

#undef F
				break;

			case RTS2_DATA_USHORT:
#define F ((uint16_t *) f)
				for (i = 0; i < n; i++)
					F[i] = (uint16_t) fabs(F[i] - M);
				qsort (f, n, sizeof(uint16_t), cmpuint16_t);
				if (n % 2)
					S = F[n / 2];
				else
					S = (F[n / 2] + F[n / 2 + 1]) / 2;

#undef F
				break;
		}
		*sigma = S * sf;
	}

	delete[] f;

	return M;
}

void XFitsImage::buildWindow ()
{
	XTextProperty window_title;
	char *cameraName;

	Status ret;

	colormap = DefaultColormap (display, DefaultScreen (display));

	// allocate colormap..
	for (int i = 0; i < 256; i++)
	{
		rgb[i].red = (unsigned short) (65536 * (1.0 * i / 256));
		rgb[i].green = (unsigned short) (65536 * (1.0 * i / 256));
		rgb[i].blue = (unsigned short) (65536 * (1.0 * i / 256));
		rgb[i].flags = DoRed | DoGreen | DoBlue;

		ret = XAllocColor (display, colormap, rgb + i);
		if (!ret)
			throw rts2core::Error ("cannot allocate color");
	}
	rgb[256].red = USHRT_MAX;
	rgb[256].green = 0;
	rgb[256].blue = 0;
	rgb[256].flags = DoRed | DoGreen | DoBlue;

	rgb[257].red = 0;
	rgb[257].green = USHRT_MAX;
	rgb[257].blue = 0;
	rgb[257].flags = DoRed | DoGreen | DoBlue;

	ret = XAllocColor (display, colormap, &rgb[256]);
	if (!ret)
		throw rts2core::Error ("cannot allocate foreground color");

	window = XCreateWindow (display, DefaultRootWindow (display), 0, 0, 100, 100, 0, depth, InputOutput, visual, 0, &xswa);
	pixmap = XCreatePixmap (display, window, windowWidth, windowHeight, depth);

	gc = XCreateGC (display, pixmap, 0, &gvc);
	XSelectInput (display, window, KeyPressMask | ButtonPressMask | ExposureMask | PointerMotionMask);
	XMapRaised (display, window);

	cameraName = new char[strlen (connection->getName ()) + 1];
	strcpy (cameraName, connection->getName ());

	XStringListToTextProperty (&cameraName, 1, &window_title);
	XSetWMName (display, window, &window_title);

	delete[] cameraName;
}

// called to rebuild window on the screen
void XFitsImage::rebuildWindow ()
{
	if (gc)
		XFreeGC (display, gc);
	if (pixmap)
		XFreePixmap (display, pixmap);
	pixmap = XCreatePixmap (display, window, windowWidth, windowHeight, depth);

	if (ximage)
	{
		XDestroyImage (ximage);
		ximage = NULL;
	}
	gc = XCreateGC (display, pixmap, 0, &gvc);
}

void XFitsImage::drawCenterCross (int xc, int yc)
{
	// draw cross at center
	XDrawLine (display, pixmap, gc, xc - 10, yc, xc - 2, yc);
	XDrawLine (display, pixmap, gc, xc + 10, yc, xc + 2, yc);
	XDrawLine (display, pixmap, gc, xc, yc - 10, xc, yc - 2);
	XDrawLine (display, pixmap, gc, xc, yc + 10, xc, yc + 2);
}

void XFitsImage::drawCross1 ()
{
	XSetForeground (display, gc, rgb[256].pixel);
	int rectNum;
	int i;
	int xc, yc;
	XRectangle *rectangles;

	rectNum = (pixmapWidth / 40 > pixmapHeight / 40) ? pixmapHeight / 40 : pixmapWidth / 40;
	rectangles = new XRectangle[rectNum];

	XRectangle *rect = rectangles;

	xc = pixmapWidth / 2;
	yc = pixmapHeight / 2;

	drawCenterCross (xc, yc);

	for (i = 0; i < rectNum;)
	{
		i++;
		xc -= 20;
		yc -= 20;
		rect->x = xc;
		rect->y = yc;
		rect->width = i * 40;
		rect->height = i * 40;
		rect++;
	}
	XDrawRectangles (display, pixmap, gc, rectangles, rectNum);

	delete[]rectangles;
}

void XFitsImage::drawCross2 ()
{
	XSetForeground (display, gc, rgb[256].pixel);
	int arcNum;
	int i;
	int xc, yc;
	XArc *arcs;

	arcNum = (pixmapWidth / 40 > pixmapHeight / 40) ? pixmapHeight / 40 : pixmapWidth / 40;
	arcs = new XArc[arcNum];

	XArc *arc = arcs;

	xc = pixmapWidth / 2;
	yc = pixmapHeight / 2;

	drawCenterCross (xc, yc);

	for (i = 0; i < arcNum; )
	{
		i++;
		xc -= 20;
		yc -= 20;
		arc->x = xc;
		arc->y = yc;
		arc->width = i * 40;
		arc->height = i * 40;
		arc->angle1 = 0;
		arc->angle2 = 23040;
		arc++;
	}
	XDrawArcs (display, pixmap, gc, arcs, arcNum);

	delete[]arcs;
}

void XFitsImage::drawCross3 ()
{
	static XPoint points[5];
	int xc, yc;

	XSetForeground (display, gc, rgb[256].pixel);
	// draw lines on surrounding
	int w = pixmapWidth / 7;
	int h = pixmapHeight / 7;
	XDrawLine (display, pixmap, gc, 0, 0, w, h);
	XDrawLine (display, pixmap, gc, pixmapWidth, 0, pixmapWidth - w, h);
	XDrawLine (display, pixmap, gc, 0, pixmapHeight, w, pixmapHeight - h);
	XDrawLine (display, pixmap, gc, pixmapWidth, pixmapHeight, pixmapWidth - w, pixmapHeight - h);
	XDrawRectangle (display, pixmap, gc, pixmapWidth / 4, pixmapHeight / 4, pixmapWidth / 2, pixmapHeight / 2);
	// draw center..
	xc = pixmapWidth / 2;
	yc = pixmapHeight / 2;
	w = pixmapWidth / 10;
	h = pixmapHeight / 10;
	points[0].x = xc - w;
	points[0].y = yc;

	points[1].x = xc;
	points[1].y = yc + h;

	points[2].x = xc + w;
	points[2].y = yc;

	points[3].x = xc;
	points[3].y = yc - h;

	points[4].x = xc - w;
	points[4].y = yc;

	XDrawLines (display, pixmap, gc, points, 5, CoordModeOrigin);
	XDrawLine (display, pixmap, gc, xc, yc - pixmapHeight / 15, xc, yc + pixmapHeight / 15);
}

void XFitsImage::drawCross4 ()
{
	XSetForeground (display, gc, rgb[256].pixel);
	int i;

	drawCenterCross (pixmapWidth / 2, pixmapHeight / 2);

	for (i = 0; i < pixmapWidth; i += 20)
	{
		XDrawLine (display, pixmap, gc, i, 0, i, pixmapHeight);
		XDrawLine (display, pixmap, gc, 0, i, pixmapHeight, i);
	}
}

void XFitsImage::drawMarkers ()
{
	XSetForeground (display, gc, rgb[257].pixel);

	for (std::list <Marker>::iterator i = markers.begin (); i != markers.end (); i++)
		drawCenterCross (i->x, i->y);
}

void XFitsImage::drawStars (rts2image::Image * image)
{
	struct rts2image::stardata *sr;
	if (!image)
		return;
	sr = image->sexResults;
	for (int i = 0; i < image->sexResultNum; i++, sr++)
	{
		XDrawArc (display, pixmap, gc, (int) sr->X - 10, (int) sr->Y - 10, 20, 20, 0, 23040);
	}
}

void XFitsImage::printInfo ()
{
	XSetBackground (display, gc, rgb[0].pixel);
	std::ostringstream _os;
	_os << "L: " << low << " M: " << median << " H: " << high << " Min: " << min << " Max: " << max;
	XDrawImageString (display, pixmap, gc, pixmapWidth / 2 - 100, 20, _os.str ().c_str (), _os.str ().length ());
	std::ostringstream _os1;
	_os1.precision (2);
	_os1 << "avg " << average;
	XDrawImageString (display, pixmap, gc, pixmapWidth / 2 - 50, 32, _os1.str ().c_str (), _os1.str ().length ());

	if (lastImage)
	{
	  	std::ostringstream _os2;
		_os2 << "[" << lastX << "," << lastY << ":" << lastSizeX << "," << lastSizeY << "] binn: " << binningsX << ":" << binningsY;  //<< " exposureTime: " << getConnection ()->getValue ("exposure")->getValueDouble ();
		XDrawImageString (display, pixmap, gc, pixmapWidth / 2 - 150, pixmapHeight - 20, _os2.str ().c_str (), _os2.str ().length ());
	}
}

void XFitsImage::printMouse ()
{
	char stringBuf[20];
	int len;
	len = snprintf (stringBuf, 20, "%04i %04i", mouseX, mouseY);
	XSetBackground (display, gc, rgb[0].pixel);
	XSetForeground (display, gc, rgb[256].pixel);
	XDrawImageString (display, pixmap, gc, 30, 30, stringBuf, len);
	if (buttonX >= 0 && buttonY >= 0)
		drawCenterCross (buttonX, buttonY);
}

void XFitsImage::redrawMouse ()
{
	XClearArea (display, window, 0, 0, 200, 40, False);
	XClearArea (display, window, buttonX - 10, buttonY - 10, 20, 20, False);
}

void XFitsImage::redraw ()
{
	// do some line-drawing etc..
	switch (crossType)
	{
		case 1:
			drawCross1 ();
			break;
		case 2:
			drawCross2 ();
			break;
		case 3:
			drawCross3 ();
			break;
		case 4:
			drawCross4 ();
			break;
	}
	if (crossType > 0)
		printInfo ();
	// draw plots over stars..
	//drawStars (getActualImage ());

	printMouse ();

	xswa.colormap = colormap;
	xswa.background_pixmap = pixmap;

	XChangeWindowAttributes (display, window, CWColormap | CWBackPixmap, &xswa);

	XClearWindow (display, window);
}
