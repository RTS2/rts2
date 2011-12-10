/*
 * Master include file.
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

/**
 * @mainpage RTS2 - Remote Telescope System, 2nd Version
 *
 * @section RTS2_goals Project goals
 *
 * The original goal of the project was simple - complete master degree. The project went
 * well pass this goal, allowing a lot more, integrating code to support more devices
 * and provide functionality it was not intended to provide at the beginning.
 *
 * Listing current functions and capabilities is beyond scope of this document. Please see
 * project Web at <a href='http://rts2.org'>rts2.org</a> for details.
 *
 * @section RTS2_philosophy Project philosophy
 *
 * The code is build form modules. Every module is a separate C++ class,
 * extending one of the available, usuall abstract classes.
 *
 * @subsection RTS2_abstract Abstract classes
 *
 * Abstract classes are used in the code to declare interfaces. For example, you will
 * find two pure virtual (declared as virtual int startExposure () = 0 in \ref camd.h)
 * methods in \ref Camera. If you subclass from the abstract class, you have to provide
 * at least empty implementation of the pure virtual methods in order to be able to compile
 * the code. This way, C++ shows you which methods you should implement for a new driver to
 * be added into the system.
 */
