#!/usr/bin/perl -w

use strict;

use LWP::UserAgent;

my $page = "http://cfa-www.harvard.edu/iau/Ephemerides/Comets/Soft00Cmt.txt";

sub julian_day
{
  my $year = shift;
  my $month = shift;
  my $day = shift;

# check for month = January or February */
  if ($month < 3 ) {
        $year--;
	$month += 12;
  }

  my $a = $year / 100;

  my $b = 0;

# check for Julian or Gregorian calendar (starts Oct 4th 1582) */
  if ($year > 1582 || ($year == 1582 && ($month > 10 || ($month == 10 && $day >= 4)))) {
     # Gregorian calendar    
     $b = 2 - $a + ($a / 4);
  }
  return int(365.25 * ($year + 4716)) + int(30.6001 * ($month + 1))
    + $day + $b - 1524.5;
}

print "-- Getting $page..";

my $ua = LWP::UserAgent->new;
$ua->agent("RTS2/0.1");

# Create a request
my $req = HTTP::Request->new(GET => $page);

# Pass request to the user agent and get a response back
my $res = $ua->request($req);

die $res->status_line unless ($res->is_success);

print "OK\n";

my @res_arr = split (/\n/, $res->content);

while (my $row = shift @res_arr)
{
  chomp $row;
#  print $row;
  $row =~ s/^\s+//;
  $row =~ s/\s+$//;
  my @hodn = split (/\s+/, $row);

  my $design = $hodn[0];
  my $tp_y = $hodn[1];
  my $tp_m = $hodn[2];
  my $tp_d = $hodn[3];
  my $a = $hodn[4];
  my $e = $hodn[5];
  my $w = $hodn[6];
  my $omega = $hodn[7];
  my $i = $hodn[8];
  if ($hodn[9] =~ /^\d+$/)
  {
    shift @hodn;
  }
  my $m_1 = $hodn[9];
  my $m_2 = $hodn[10];
  my $name = "";
  for (my $j = 11; $j <= $#hodn; $j++)
  {
    $name .= "$hodn[$j] ";
  }

  print "SELECT ell_update ('$name', $a, $e, $i, $w, $omega, 0, " .
    julian_day ($tp_y, $tp_m, $tp_d) . ", $m_1, $m_2);\n";
}
