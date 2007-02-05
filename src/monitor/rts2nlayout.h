#ifndef __RTS2_NLAYOUT__
#define __RTS2_NLAYOUT__

/**
 * Holds informations which are used for NCURSER windows layout.
 */
class Rts2NLayout
{
public:
  Rts2NLayout ()
  {
  }
  virtual ~ Rts2NLayout (void)
  {
  }
  virtual void resize (int x, int y, int w, int h) = 0;
};

class Rts2NLayoutBlock:public Rts2NLayout
{
private:
  Rts2NLayout * layoutA;
  Rts2NLayout *layoutB;
  // if true, represents vertical windows
  bool vertical;
  // windows ratio, in percentages for first window
  int ratio;
public:
    Rts2NLayoutBlock (Rts2NLayout * in_laA, Rts2NLayout * in_laB,
		      bool in_vertical, int in_ratio)
  {
    layoutA = in_laA;
    layoutB = in_laB;
    vertical = in_vertical;
    ratio = in_ratio;
  }
  virtual ~ Rts2NLayoutBlock (void)
  {
    delete layoutA;
    delete layoutB;
  }
  void setLayoutA (Rts2NLayout * in_laA)
  {
    layoutA = in_laA;
  }
  void setLayoutB (Rts2NLayout * in_laB)
  {
    layoutB = in_laB;
  }
  virtual void resize (int x, int y, int w, int h);
};

#endif /*! __RTS2_NLAYOUT__ */
