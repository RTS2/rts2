namespace cpp rts2

struct RaDec {
  1: double ra,
  2: double dec
}

struct MountInfo {
  1: RaDec ORI,
  2: RaDec TEL
}

service Mount {
  MountInfo info(),
  i32 Slew(1: RaDec target)
}
