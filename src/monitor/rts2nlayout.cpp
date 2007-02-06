#include "rts2nlayout.h"

void
Rts2NLayoutBlock::resize (int x, int y, int w, int h)
{
  int tmp_i;
  if (vertical)
    {
      tmp_i = (int) ((double) w * ratio / 100.0);
      layoutA->resize (x, y, tmp_i, h);
      layoutB->resize (x + tmp_i, y, w - tmp_i, h);
    }
  else
    {
      tmp_i = (int) ((double) h * ratio / 100.0);
      layoutA->resize (x, y, w, tmp_i);
      layoutB->resize (x, y + tmp_i, w, h - tmp_i);
    }
}

void
Rts2NLayoutBlockFixedA::resize (int x, int y, int w, int h)
{
  int tmp_i = getRatio ();
  if (isVertical ())
    {
      if (w < tmp_i)
	tmp_i = w;
      getLayoutA ()->resize (x, y, tmp_i, h);
      getLayoutB ()->resize (x + tmp_i, y, w - tmp_i, h);
    }
  else
    {
      if (h < tmp_i)
	tmp_i = h;
      getLayoutA ()->resize (x, y, w, tmp_i);
      getLayoutB ()->resize (x, y + tmp_i, w, h - tmp_i);
    }
}

void
Rts2NLayoutBlockFixedB::resize (int x, int y, int w, int h)
{
  int tmp_i = getRatio ();
  if (isVertical ())
    {
      if (w < tmp_i)
	tmp_i = w;
      getLayoutA ()->resize (x, y, w - tmp_i, h);
      getLayoutB ()->resize (x + w - tmp_i, y, tmp_i, h);
    }
  else
    {
      if (h < tmp_i)
	tmp_i = h;
      getLayoutA ()->resize (x, y, w, h - tmp_i);
      getLayoutB ()->resize (x, y + h - tmp_i, w, tmp_i);
    }
}
