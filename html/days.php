<?
include "fn.php";
	include "simple_query.php";

	function table_start ($wday) {
		echo "<table style='border:solid black 2pt' width='100%'>\n\t<th><tr>\n";

		$days = array ('Mo', 'Tue', 'Wed', 'Thr', 'Fri', 'Sat', 'Sun');

		for ($i = 0; $i < 7; $i++) {
			echo "<td width='70'>$days[$i]</td>"; 
		}
		echo "</tr></th><tr>";
		for ($i = 1; $i < $wday; $i++) 
		{
			echo "\t\t<td>-</td>\n";	
		}
	}

	function table_end ($wday) {
		for ($i = $wday; $i < 8; $i++) {
			echo "\t\t<td>-</td>\n";	
		}
		echo "\t</tr>\n</table>\n";
	}

	$con = pg_connect ("dbname=stars");

	$year = $_REQUEST["year"];

	if (!is_numeric($year)) {
		echo "<b>Invalid year value ($year).</b>";
		exit (1);
	}

	hlavicka ("BART - year statistics", "BART - year statistics", "", "ok");

	$obs = pg_query ("SELECT count(*), obs_night, obs_month from observations_nights where obs_year = $year group by obs_night,obs_month order by obs_month,obs_night;");
	$img = pg_query ("SELECT count(*), img_night, img_month from images_nights where img_year = $year group by img_night, img_month order by img_month, img_night;");

	$wday = date ("w", mktime (0,0,0,1,1,$year));

	$month_days = array (31,$year % 4 ? 28:29,31,30,31,30,31,31,30,31,30,31);

	$obs_row = pg_fetch_row ($obs);
	$img_row = pg_fetch_row ($img);

	echo "<table width='100%'><tr><td valign='top'>\n";

	for ($last_month = 1; $last_month < 13; $last_month++) {
		echo "<p>Month: $last_month</p>"; 
		table_start ($wday);
		for ($last_day = 1; $last_day <= $month_days[$last_month - 1]; $last_day++) {
			echo "\t\t<td height='50' style='border: solid darkblue 1pt;'><table><tr><td>$last_day";
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
				echo "</td><td><font size='-1'>$obss</font><br><font size='-1'>$imgs</font>";
			
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

	echo "</td></tr></table>\n";

	pg_close ($con);
	
	konec ();
?>	
