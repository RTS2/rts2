#!/usr/bin/perl -w
use strict;
use POSIX;

my $title = undef;
my $id = undef;
my $seqn = undef;
my $ra = undef;
my $dec = undef;
my $grb_date = 0;
my $in_wxm_ra = 0;
my $in_wxm_dec = 0;

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
	print "$id $seqn $ra $dec $grb_date $title\n" if ($id && $seqn && $ra && $dec && $title);
	my $bacmail;
	open $bacmail, "|/home/petr/rts2/src/grb/bacodine/bacmail";
	print $bacmail "$id $seqn $ra $dec $grb_date $title\n" if ($id && $seqn && $ra && $dec && $title);
	close $bacmail;
} else {
	print "BACODINE no position\n";
}
