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

enum TrackType {
     OFF = 0,
     SIDEREAL = 1,
     NONSIDEREAL = 2
}

struct MountInfo {
  1: double infotime,
  2: RaDec ORI,                 // target coordinates
  3: RaDec offsets,             // RA DEC offsets
  4: AltAz altAzOffsets,        // AltAz offsets
  5: RaDec TEL,                 // Telescope coordinates
  6: AltAz HRZ,                 // Telescope horizontal coordinates
  7: double JulianDay,          // Telescope julian date
  8: double Focus,              // The current focus value
  9: TrackType TrackingType,    // The type of tracking being used
  10: RaDec ns_params,          // The parameters used for non-sidereal tracking
  11: bool autoguide_state,     // The autoguide state
  12: bool slewing,             // Whether the telescope is currently slewing to a new position
  13: i32 instrumentport,       // The instrument port in use
}

struct DerotatorInfo {
  1: double infotime,
  2: double PA,                 // parallactic angle offset
  3: double offset,             // derotator offset
  4: double current,
  5: double target
}

struct DomeInfo {
  1: bool domestatus,	        // the dome status: on (true) or off (false)
  2: bool lights,               // the dome light state: on (true) or off (false)
  3: bool domeshutters,
  4: bool mirrorcovers,
  5: double DomeAngle,          // The current dome angle
}

exception RTS2Exception {
          1: string message;
}

service ObservatoryService {
  MountInfo infoMount(),
  DerotatorInfo infoDerotator(),
  DomeInfo infoDome(),
  i32 Slew(1: RaDec target) throws (1:RTS2Exception e),     // Slew to the given RA and dec
  i32 AdjustRADec(1: RaDec delta) throws (1: RTS2Exception e),   // Adjust the RA and dec by delta
  i32 SlewAltAz(1: AltAz pos) throws (1: RTS2Exception e),   // Slew to the given altitude and azimuth
  i32 AdjustAltAz(1: AltAz delta),                      // Adjust the altitude and azimuth by delta
  i32 RotateDome(1: double angle),                      // Rotate the dome to the specified angle
  i32 TrackType(1: TrackType val),                      // Set tracking to OFF, SIDEREAL or NONSIDEREAL
  i32 NonSiderealParams(1: RaDec ns_diff),              // Set the differential RA and dec
                                                        // used for non-sidereal tracking
  i32 AutoGuide(1: bool val),                           // Set autoguide on (true) or off (false)
  i32 SetFocus(1: double focus),                        // Move the focus to an absolute position
  i32 Park(),                                           // Park the telescope
  i32 DomeStatus(1: bool val)                           // Switch the dome on (true) or off (false)
  i32 DomeLights(1: bool val),                          // Switch the dome lights on (true)
                                                        // or off (false)
  i32 DomeShutters(1: bool val),                        // Open (true) or close (false)
                                                        // the dome shutters
  i32 MirrorCovers(1: bool val),                        // Open (true) or close (false)
                                                        // the mirror covers
  i32 SelectPort(1: i32 port),                          // Choose the instrument port 
  i32 Reset()                                           // Reset all subsystems
  i32 Abort()                                           // Immediately stop all telescope slewing
}
