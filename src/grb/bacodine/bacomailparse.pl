#!/usr/bin/perl -w
use strict;
use libnova;
use POSIX;

my $title = undef;
my $id = undef;
my $seqn = undef;
my $ra = undef;
my $dec = undef;
my $grb_date = 0;
my $in_wxm_ra = 0;
my $in_wxm_dec = 0;

my $lng = 345.383;
my $lat = 49.9;

sub get_time_from_JD {
	my $JD = shift;
	$JD = $JD - int ($JD) + 0.5;
	$JD-- if ($JD > 1);
	$JD = $JD * 24.0;
	return sprintf ("%02i:%02i", int($JD), int(($JD - int ($JD)) * 60.0));
}

while (<>) {
	m#^TITLE:\s*(.*)$# and $title = $1;
	m#^TRIGGER_NUM:\s*([0-9]+),.*Seq_Num:.*([0-9]+)$# and $id = $1 and $seqn = $2;
	if (m#^GRB_DATE:\s*.*([0-9]{1,2})/([0-9]{1,2})/([0-9]{1,2})$#)
	{
		$grb_date += mktime (0,0,0,$3, $2 - 1,$1 + 100);
	}
	m#^GRB_TIME:\s*([0-9]*)(?:\.[0-9]*) SOD.*UT$# and $grb_date += $1;
	m#^.*_CNTR_RA:# and $in_wxm_ra = 1;
	m#^.*_CNTR_DEC:# and $in_wxm_dec = 1;
	if ($in_wxm_ra && m#\s*([+-]?[0-9]*\.[0-9]*)d.*\((.*)\)(,?)$#)
	{
		$ra = $1 if ($2 eq 'current');
		$in_wxm_ra = 0 unless ($3 eq ',');
	}
	if ($in_wxm_dec && m#\s*([+-]?[0-9]*\.[0-9]*)d.*\((.*)\)(,?)$#)
	{
		$dec = $1 if ($2 eq 'current');
		$in_wxm_dec = 0 unless ($3 eq ',');
	}
}
if ($id && $seqn && $ra && $dec && $title)
{
	my $JD = libnova::get_julian_from_sys ();
	my $r = libnova::get_object_rise ($JD, $lng, $lat, $ra, $dec);
	my $t = libnova::get_object_transit ($JD, $lng, $lat, $ra, $dec);
	my $s = libnova::get_object_set ($JD, $lng, $lat, $ra, $dec);

	$r = get_time_from_JD($r);
	$t = get_time_from_JD($t);
	$s = get_time_from_JD($s);
	
	my $st = libnova::get_mean_sidereal_time ($JD);
	my $az = libnova::get_az_from_equ_sidereal_time ($st, $lng, $lat, $ra, $dec);
	my $alt = libnova::get_alt_from_equ_sidereal_time ($st, $lng, $lat, $ra, $dec);
	if ($id && $seqn && $ra && $dec && $title)
	{
		my $grb_time = POSIX::strftime "%H:%M:%S %e%b", gmtime ($grb_date);
		my $bacmail;
		open $bacmail, "|/home/petr/rts2/src/grb/bacodine/rts2-bacmail >> /home/petr/bac_out";
		print $bacmail "$id $seqn $ra $dec $grb_date $title\n";
		close $bacmail;
#		print "mail kvasar\@mujoskar.cz petr -s '$id $seqn $ra $dec $grb_time $title R$r T$t S$s AZ" . int($az). " ALT" . int($alt) . "'";
		system "mail kvasar\@mujoskar.cz petr -s '$id $seqn $ra $dec $grb_time $title R$r T$t S$s AZ" . int($az). " ALT" . int($alt) . "'";
#		my $gnokii;
#		open $gnokii, "|/usr/local/bin/gnokii --sendsms +420602105255";
#		print $gnokii "ID:$id-$seqn\nRD:$ra $dec\nDA:$grb_time\nAZAL:" . int($az) . " " . int($alt) . "\n" . $title;
#		close $gnokii;
		my $smsd;
		open $smsd, ">/var/spool/smsd/grb.$$.1";
#		print $smsd "+420602105255\nID:$id-$seqn RD:$ra $dec DA:$grb_time AZAL:" . int($az) . " " . int($alt) . " " . $title;
		print $smsd "+34617840945\nID:$id-$seqn RD:$ra $dec DA:$grb_time AZAL:" . int($az) . " " . int($alt) . " " . $title;
		close $smsd;
		open $smsd, ">/var/spool/smsd/grb.$$.3";
		print $smsd "+34616031753\nID:$id-$seqn RD:$ra $dec DA:$grb_time AZAL:" . int($az) . " " . int($alt) . " " . $title;
		close $smsd;
		open $smsd, ">/var/spool/smsd/grb.$$.2";
		print $smsd "+420737500268\nID:$id-$seqn RD:$ra $dec DA:$grb_time AZAL:" . int($az) . " " . int($alt) . " " . $title;
		close $smsd;
	}
} else {
	print "BACODINE no position\n";
}
