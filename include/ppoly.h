// Copyright 2000 softSurfer, 2012 Dan Sunday
// This code may be freely used and modified for any purpose
// providing that this copyright notice is included with it.
// SoftSurfer makes no warranty for this code, and cannot be held
// liable for any real or imagined damage resulting from its use.
// Users of this code must verify correctness for their application.
 
#include <vector>
#include "Point.hpp"

// cn_PnPoly(): crossing number test for a point in a polygon
//      Input:   P = a point,
//               V[] = vertex points of a polygon with V[n]=V[0]
//      Return:  0 = outside, 1 = inside
// This code is patterned after [Franklin, 2000]
int cn_PnPoly( Point P, const std::vector<Point> &V );

// wn_PnPoly(): winding number test for a point in a polygon
//      Input:   P = a point,
//               V = vertex points of a polygon with V[n]=V[0]
//      Return:  wn = the winding number (=0 only when P is outside)
int wn_PnPoly( Point P, const std::vector<Point> &V );
