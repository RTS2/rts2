#include "median.h"

void
swap (double *a, int i1, int i2)
{
  double val;
  val = a[i1];
  a[i1] = a[i2];
  a[i2] = val;
}

int
partition (double *a, int left, int right)
{
  double pivotValue;
  int pivotIndex = (left + right) / 2;
  int storeIndex = left;
  int i;
  pivotValue = a[pivotIndex];
  swap (a, pivotIndex, right);	// Move pivot to end
  for (i = left; i < right; i++)
    {
      if (a[i] <= pivotValue)
	{
	  swap (a, storeIndex, i);
	  storeIndex++;
	}
    }
  swap (a, right, storeIndex);	// Move pivot to its final place
  return storeIndex;
}

int
quicksort (double *values, int left, int right)
{
  if (right > left)
    {
      int pivotNewIndex;
      pivotNewIndex = partition (values, left, right);
      quicksort (values, left, pivotNewIndex - 1);
      quicksort (values, pivotNewIndex + 1, right);
    }
}

// can change values order, return median value
double
get_median (double *values, int size)
{
  // sort
  int i;
  quicksort (values, 0, size - 1);
  // get median
  if (size % 2 == 1)
    {
      return (values[size / 2]);
    }
  else
    {
      size /= 2;
      return (values[size] + values[size + 1]) / 2;
    }
}
