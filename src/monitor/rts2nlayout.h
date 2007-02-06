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
  Rts2NLayout *getLayoutA ()
  {
    return layoutA;
  }
  Rts2NLayout *getLayoutB ()
  {
    return layoutB;
  }
  virtual void resize (int x, int y, int w, int h);
  int getRatio ()
  {
    return ratio;
  }
  bool isVertical ()
  {
    return vertical;
  }
};

class Rts2NLayoutBlockFixedA:public Rts2NLayoutBlock
{
public:
  Rts2NLayoutBlockFixedA (Rts2NLayout * in_laA, Rts2NLayout * in_laB,
			  bool in_vertical,
			  int sizeA):Rts2NLayoutBlock (in_laA, in_laB,
						       in_vertical, sizeA)
  {
  }
  virtual ~ Rts2NLayoutBlockFixedA (void)
  {
  }
  virtual void resize (int x, int y, int w, int h);
};

class Rts2NLayoutBlockFixedB:public Rts2NLayoutBlock
{
public:
  Rts2NLayoutBlockFixedB (Rts2NLayout * in_laA, Rts2NLayout * in_laB,
			  bool in_vertical,
			  int sizeB):Rts2NLayoutBlock (in_laA, in_laB,
						       in_vertical, sizeB)
  {
  }
  virtual ~ Rts2NLayoutBlockFixedB (void)
  {
  }
  virtual void resize (int x, int y, int w, int h);
};

#endif /*! __RTS2_NLAYOUT__ */
