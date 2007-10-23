/* 
 * Telescope model terms.
 * Copyright (C) 2003-2006 Martin Jelinek <Martin Jelinek <mates@iaa.es>@iaa.es>
 * Copyright (C) 2006-2007 Petr Kubanek <petr@kubanek,net>
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

#ifndef __RTS2_MODELTERM__
#define __RTS2_MODELTERM__

#include "telmodel.h"

/**
 * @file
 * Include classes for various TPoint terms.
 *
 * @defgroup RTS2TPointTerm Modelling terms for TPOINT.
 */

class Rts2ObsConditions;

/**
 * Represents model term. Child of that class are created
 * in Rts2TelModel::load, and used in apply functions.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2ModelTerm
{
private:
  // term name
  std::string name;
protected:
  double corr;			// corection parameter, always in degrees!
  double sigma;			// model sigma
public:
    Rts2ModelTerm (const char *in_name, double in_corr, double in_sigma)
  {
    name = std::string (in_name);
    corr = in_corr;
    sigma = in_sigma;
  }
  virtual ~ Rts2ModelTerm (void)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions) = 0;
  virtual void reverse (struct ln_equ_posn *pos,
			Rts2ObsConditions * obs_conditions)
  {
    struct ln_equ_posn pos_old;
    pos_old.ra = pos->ra;
    pos_old.dec = pos->dec;
    apply (pos, obs_conditions);
    pos->ra = 2 * pos_old.ra - pos->ra;
    pos->dec = 2 * pos_old.dec - pos->dec;
  }

  friend std::ostream & operator << (std::ostream & os, Rts2ModelTerm * term);
};

std::ostream & operator << (std::ostream & os, Rts2ModelTerm * term);

/**
 * Polar axis misalignment in elevation term.
 *
 * @author Martin Jelinek <Martin Jelinek <mates@iaa.es>@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermME:public Rts2ModelTerm
{
public:
  Rts2TermME (double in_corr, double in_sigma):Rts2ModelTerm ("ME", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Polar axis misalignment in azimuth term.
 *
 * @author Martin Jelinek <Martin Jelinek <mates@iaa.es>@iaa.es>a
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermMA:public Rts2ModelTerm
{
public:
  Rts2TermMA (double in_corr, double in_sigma):Rts2ModelTerm ("MA", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Index error in hour angle.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermIH:public Rts2ModelTerm
{
public:
  Rts2TermIH (double in_corr, double in_sigma):Rts2ModelTerm ("IH", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Index error in declination.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermID:public Rts2ModelTerm
{
public:
  Rts2TermID (double in_corr, double in_sigma):Rts2ModelTerm ("ID", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * East-west collimation error.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermCH:public Rts2ModelTerm
{
public:
  Rts2TermCH (double in_corr, double in_sigma):Rts2ModelTerm ("CH", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * HA/Dec non-perpendicularity.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermNP:public Rts2ModelTerm
{
public:
  Rts2TermNP (double in_corr, double in_sigma):Rts2ModelTerm ("NP", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Step size in h (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermPHH:public Rts2ModelTerm
{
public:
  Rts2TermPHH (double in_corr, double in_sigma):Rts2ModelTerm ("PHH", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Step size in declination (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 */
class Rts2TermPDD:public Rts2ModelTerm
{
public:
  Rts2TermPDD (double in_corr, double in_sigma):Rts2ModelTerm ("PDD", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Aux1 to h (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermA1H:public Rts2ModelTerm
{
public:
  Rts2TermA1H (double in_corr, double in_sigma):Rts2ModelTerm ("A1H", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Aux1 to in declination (for Paramount, where it's unsure).
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermA1D:public Rts2ModelTerm
{
public:
  Rts2TermA1D (double in_corr, double in_sigma):Rts2ModelTerm ("A1D", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Tube flexture.
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermTF:public Rts2ModelTerm
{
public:
  Rts2TermTF (double in_corr, double in_sigma):Rts2ModelTerm ("TF", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Tube flexure (tangent)
 *
 * @author Martin Jelinek <mates@iaa.es>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermTX:public Rts2ModelTerm
{
public:
  Rts2TermTX (double in_corr, double in_sigma):Rts2ModelTerm ("TX", in_corr,
							      in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * HA centering error, cosine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermHCEC:public Rts2ModelTerm
{
public:
  Rts2TermHCEC (double in_corr, double in_sigma):Rts2ModelTerm ("HCEC",
								in_corr,
								in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * HA centering error, sine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermHCES:public Rts2ModelTerm
{
public:
  Rts2TermHCES (double in_corr, double in_sigma):Rts2ModelTerm ("HCES",
								in_corr,
								in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Dec centering error, cosine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermDCEC:public Rts2ModelTerm
{
public:
  Rts2TermDCEC (double in_corr, double in_sigma):Rts2ModelTerm ("DCEC",
								in_corr,
								in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Dec centering error, sine component.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermDCES:public Rts2ModelTerm
{
public:
  Rts2TermDCES (double in_corr, double in_sigma):Rts2ModelTerm ("DCES",
								in_corr,
								in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Declination axis bending.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermDAB:public Rts2ModelTerm
{
public:
  Rts2TermDAB (double in_corr, double in_sigma):Rts2ModelTerm ("DAB", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/**
 * Declination axis flop.
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermDAF:public Rts2ModelTerm
{
public:
  Rts2TermDAF (double in_corr, double in_sigma):Rts2ModelTerm ("DAF", in_corr,
							       in_sigma)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};


typedef enum
{ SIN, COS, NOT } sincos_t;

/**
 * Class which calculate harmonics terms.
 * It have to construct function from name, using sin & cos etc..
 *
 * @author Petr Kubanek <petr@kubanek.net>
 *
 * @addgroup RTS2TPointTerm
 */
class Rts2TermHarmonics:public Rts2ModelTerm
{
private:
  char resType;
  sincos_t func[2];
  char param[2];
  int mul[2];

  const char *getFunc (const char *in_func, int i);
  double getValue (struct ln_equ_posn *pos,
		   Rts2ObsConditions * obs_conditions, int i);
  double getMember (struct ln_equ_posn *pos,
		    Rts2ObsConditions * obs_conditions, int i);
public:
    Rts2TermHarmonics (double in_corr, double in_sigma, const char *in_name);
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

#endif /*! __RTS2_MODELTERM__ */
