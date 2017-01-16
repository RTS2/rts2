namespace cpp rts2

// ctime - seconds from 1.1.1970 UTC

// All units (including RA) are in degrees
// Az is 0 north, 90 east (standard, not IAU/RTS2)
struct RaDec {
  1: double ra,
  2: double dec
}

struct AltAz {
  1: double az,
  2: double alt
}

struct MountInfo {
  1: double infotime,
// target coordinates
  2: RaDec ORI,
// RA DEC offsets
  3: RaDec offsets,
// AltAz offsets
  4: AltAz altAzOffsets,
// Telescope coordinates
  5: RaDec TEL,
// Telescope horizontal coordinates
  6: AltAz HRZ,
// Telescope julian date
  7: double JulianDay
}

struct DerotatorInfo {
  1: double infotime,
// parallactic angle offset
  2: double PA,
// derotator offset
  3: double offset,
  4: double current,
  5: double target
}

service ObservatoryService {
  MountInfo infoMount(),
  DerotatorInfo infoDerotator(),
  i32 Slew(1: RaDec target),
  i32 Park()
}
