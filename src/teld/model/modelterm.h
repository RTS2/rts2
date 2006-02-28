#ifndef __RTS2_MODELTERM__
#define __RTS2_MODELTERM__

#include "telmodel.h"

class Rts2ObsConditions;

/*!
 * Represents model term. Child of that class are created
 * in Rts2TelModel::load, and used in apply functions.
 *
 * @author petr
 */
class Rts2ModelTerm
{
private:
  // term name
  const char *name;
protected:
  double corr;			// corection parameter, always in degrees!
public:
    Rts2ModelTerm (const char *in_name, double in_corr)
  {
    name = in_name;
    corr = in_corr;
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

/*!
 * Polar axis misalignment in elevation term.
 *
 * @author mates
 */
class Rts2TermME:public Rts2ModelTerm
{
public:
  Rts2TermME (double in_corr):Rts2ModelTerm ("ME", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Polar axis misalignment in azimuth term.
 *
 * @author mates
 */
class Rts2TermMA:public Rts2ModelTerm
{
public:
  Rts2TermMA (double in_corr):Rts2ModelTerm ("MA", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Index error in hour angle.
 *
 * @author mates
 */
class Rts2TermIH:public Rts2ModelTerm
{
public:
  Rts2TermIH (double in_corr):Rts2ModelTerm ("IH", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Index error in declination.
 *
 * @author mates
 */
class Rts2TermID:public Rts2ModelTerm
{
public:
  Rts2TermID (double in_corr):Rts2ModelTerm ("ID", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * East-west collimation error.
 *
 * @author mates
 */
class Rts2TermCH:public Rts2ModelTerm
{
public:
  Rts2TermCH (double in_corr):Rts2ModelTerm ("CH", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * HA/Dec non-perpendicularity.
 *
 * @author mates
 */
class Rts2TermNP:public Rts2ModelTerm
{
public:
  Rts2TermNP (double in_corr):Rts2ModelTerm ("NP", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Step size in h (for Paramount, where it's unsure).
 *
 * @author mates
 */
class Rts2TermPHH:public Rts2ModelTerm
{
public:
  Rts2TermPHH (double in_corr):Rts2ModelTerm ("PHH", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Step size in declination (for Paramount, where it's unsure).
 *
 * @author mates
 */
class Rts2TermPDD:public Rts2ModelTerm
{
public:
  Rts2TermPDD (double in_corr):Rts2ModelTerm ("PDD", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Aux1 to h (for Paramount, where it's unsure).
 *
 * @author mates
 */
class Rts2TermA1H:public Rts2ModelTerm
{
public:
  Rts2TermA1H (double in_corr):Rts2ModelTerm ("A1H", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Aux1 to in declination (for Paramount, where it's unsure).
 *
 * @author mates
 */
class Rts2TermA1D:public Rts2ModelTerm
{
public:
  Rts2TermA1D (double in_corr):Rts2ModelTerm ("A1D", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Tube flexture.
 *
 * @author mates
 */
class Rts2TermTF:public Rts2ModelTerm
{
public:
  Rts2TermTF (double in_corr):Rts2ModelTerm ("TF", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

/*!
 * Tube ?
 *
 * @author mates
 */
class Rts2TermTX:public Rts2ModelTerm
{
public:
  Rts2TermTX (double in_corr):Rts2ModelTerm ("TX", in_corr)
  {
  }
  virtual void apply (struct ln_equ_posn *pos,
		      Rts2ObsConditions * obs_conditions);
};

#endif /*! __RTS2_MODELTERM__ */
