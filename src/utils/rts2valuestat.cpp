#include "rts2valuestat.h"
#include "rts2conn.h"

void
Rts2ValueDoubleStat::clearStat ()
{
  numMes = 0;
  mean = nan ("f");
  min = nan ("f");
  max = nan ("f");
  stdev = nan ("f");
  valueList.clear ();
}

void
Rts2ValueDoubleStat::calculate ()
{
  if (valueList.size () == 0)
    return;
  std::vector < double >sorted = valueList;
  std::sort (sorted.begin (), sorted.end ());
  min = *(sorted.begin ());
  max = *(--sorted.end ());
  numMes = valueList.size ();
  double sum = 0;
  for (std::vector < double >::iterator iter = sorted.begin ();
       iter != sorted.end (); iter++)
    {
      sum += (*iter);
    }
  setValueDouble (sum / numMes);
  // calculate stdev
  stdev = 0;
  for (std::vector < double >::iterator iter = sorted.begin ();
       iter != sorted.end (); iter++)
    {
      sum = *iter - getValueDouble ();
      sum *= sum;
      stdev += sqrt (sum / numMes);
    }
  if ((numMes % 2) == 1)
    mean = sorted[numMes / 2];
  else
    mean = (sorted[numMes / 2 - 1] + sorted[numMes / 2]) / 2.0;
}

Rts2ValueDoubleStat::Rts2ValueDoubleStat (std::string in_val_name):Rts2ValueDouble
  (in_val_name)
{
  clearStat ();
  rts2Type |= RTS2_VALUE_DOUBLE_STAT;
}

Rts2ValueDoubleStat::Rts2ValueDoubleStat (std::string in_val_name, std::string in_description, bool writeToFits, int32_t flags):Rts2ValueDouble (in_val_name, in_description, writeToFits,
		 flags)
{
  clearStat ();
  rts2Type |= RTS2_VALUE_DOUBLE_STAT;
}

int
Rts2ValueDoubleStat::setValue (Rts2Conn * connection)
{
  if (connection->paramNextDouble (&value)
      || connection->paramNextSizeT (&numMes)
      || connection->paramNextDouble (&mean)
      || connection->paramNextDouble (&min)
      || connection->paramNextDouble (&max)
      || connection->paramNextDouble (&stdev) || !connection->paramEnd ())
    return -2;
  return 0;
}

const char *
Rts2ValueDoubleStat::getValue ()
{
  sprintf (buf, "%.20le %i %.20le %.20le %.20le %.20le", value, numMes,
	   mean, min, max, stdev);
  return buf;
}

const char *
Rts2ValueDoubleStat::getDisplayValue ()
{
  sprintf (buf, "%f %i %f %f %f %f",
	   getValueDouble (), numMes, mean, min, max, stdev);
  return buf;
}

int
Rts2ValueDoubleStat::send (Rts2Conn * connection)
{
  if (numMes != valueList.size ())
    calculate ();
  return Rts2ValueDouble::send (connection);
}

void
Rts2ValueDoubleStat::setFromValue (Rts2Value * newValue)
{
  Rts2ValueDouble::setFromValue (newValue);
  if (newValue->getValueType () == RTS2_VALUE_DOUBLE_STAT)
    {
      numMes = ((Rts2ValueDoubleStat *) newValue)->getNumMes ();
      mean = ((Rts2ValueDoubleStat *) newValue)->getMean ();
      min = ((Rts2ValueDoubleStat *) newValue)->getMin ();
      max = ((Rts2ValueDoubleStat *) newValue)->getMax ();
      stdev = ((Rts2ValueDoubleStat *) newValue)->getStdev ();
      valueList = ((Rts2ValueDoubleStat *) newValue)->getMesList ();
    }
}
