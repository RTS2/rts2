#!/usr/bin/perl -w

sub get_dec
{
  my @b = split (" ", shift);
  my $ret = abs ($b[0]) + $b[1] / 60.0 + $b[2] / 3600.0;
  if ($b[0] < 0)
  {
    return $ret * -1;
  }
  return $ret;
}

sub get_ra
{
  return get_dec (shift) * 15.0;
}

my $line = 0;

while (<>)
{
  chomp;
  my @a = split ("\t", $_);
  $a[1] = get_ra ($a[1]);
  $a[2] = get_dec ($a[2]);
  print join ("\t", $line + 20, 'f', @a, "t", "1", "0", '\N') . "\n";
  $line++
}
