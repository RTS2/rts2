<?
	require_once "fitsdb.php";

	function table_start ($wday) {
		echo "<table>\n\t<th><tr>\n";

		$days = array ('Mo', 'Tue', 'Wed', 'Thr', 'Fri', 'Sat', 'Sun');

		for ($i = 0; $i < 7; $i++) {
			echo "<td width='70' class='" . ($i % 2 ? 'even' : 'head') . "'>$days[$i]</td>"; 
		}
		echo "</tr></th><tr>";
		for ($i = 1; $i < $wday; $i++) 
		{
			echo "\t\t<td>-</td>\n";	
		}
	}

	function table_end ($wday) {
	        if ($wday > 1)
		{
			for ($i = $wday; $i < 8; $i++) {
				echo "\t\t<td>-</td>\n";	
			}
		}
		echo "\t</tr>\n</table>\n";
	}

	$months = array ("January", "February", "March", "April", "May", "June", "July", "August", "September",
		"October", "November", "December");

	function print_month ($last_month, $year, $con) {
		$t = mktime (0,0,0,$last_month,1,$year);
		$wday = date ("w", $t);
		if ($wday == 0)
			$wday = 7;

		$ustart = (date ("U", $t) / 86400);

		$obs = pg_query ($con, "SELECT count(*), obs_night, obs_month from observations_nights where obs_year = $year and obs_month = $last_month group by obs_night,obs_month order by obs_month,obs_night;");
		$img = pg_query ($con, "SELECT count(*), img_night, img_month from images_nights where img_year = $year and img_month = $last_month group by img_night, img_month order by img_month, img_night;");
	
		$obs_row = pg_fetch_row ($obs);
		$img_row = pg_fetch_row ($img);

		global $months;
		echo "<p>Month: " . $months[$last_month - 1] . "</p>"; 
		table_start ($wday);
		for ($last_day = 1; $last_day <= cal_days_in_month (CAL_GREGORIAN, $last_month, $year); $last_day++, $ustart++) {
			echo "\t\t<td height='50' class='" . ($wday % 2 ? 'even' : 'odd') . "'><table><tr><td><a href='night.php?night=$ustart'>$last_day</a>";
			$obss = 0;
			if ($obs_row && $obs_row[1] == $last_day && $obs_row[2] == $last_month) {
				$obss = $obs_row[0];
				$obs_row = pg_fetch_row ($obs);
			}
			$imgs = 0;
			if ($img_row && $img_row[1] == $last_day && $img_row[2] == $last_month) {
				$imgs = $img_row[0];
				$img_row = pg_fetch_row ($img);
			}
			if ($obss || $imgs)
			{
				echo "</td><td class='" . ($wday % 2 ? 'odd' : 'even') . "'><font size='-1'><a href='observations.php?night=$ustart'>$obss</a></font><br><font size='-1'><a href='images.php?night=$ustart'>$imgs</a></font>";
			
			}
			echo "</td></tr></table></td>\n";
			$wday ++;
			if ($wday > 7) {
				$wday = 1;
				echo "\t</tr><tr>\n";
			}
		}
		table_end ($wday);
		if ($last_month %2) {
			echo "</td>";
		} else {
			echo "</td></tr><tr>";
		}
		echo "<td valign='top'>\n";
	}

	$year = $_REQUEST["year"];

	if (!is_numeric($year)) {
		echo "<b>Invalid year value ($year).</b>";
		exit (1);
	}
	hlavicka ("BART - year statistics", "BART - year statistics", "", "ok");
?>
<p> 
In following calendar, upper indexes on days are number of observations made on
that night, lower index is number of images (with astronometry, e.g. good ones)
made on night. If you click on link, you will receive list of all observations
or images made on that night. <i>Night count from 12:00 to 12:00 next day.</i>
</p>
<table width='100%'><tr><td valign='top'>
<?
	$con = pg_connect ("dbname=stars");
	for ($last_month = 1; $last_month < 13; $last_month++) {
		print_month ($last_month, $year, $con);
	}
	echo "</td></tr></table>\n";
	pg_close ($con);
	konec ();
?>	
