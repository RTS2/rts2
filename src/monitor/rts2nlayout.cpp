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
