<?
	require_once "libnova.php";
	require_once "fitsdb.php";
	
	hlavicka ("BART - statistika", "BART - statistika snímkù", "", "ok");

//	bindTextDomain ($myPage, "./locale");
	
	$con = pg_connect ("dbname=stars");

	echo "<b>Date:</b>" . date ("r") . "<br>\n";
	$jd = get_julian_from_sys ();
	echo "<b>JD:</b>$jd<br>\n";
	echo "<b>GST:</b>" . get_mean_sidereal_time ($jd) . "<br>\n";

	echo "<b>Total number of images:</b>" . simple_query ($con, "SELECT count(*) FROM images;") . "<br>\n";
	printf ("<b>Lowest CCD temperature:</b> %0.1f <br>\n", simple_query ($con, "SELECT min(img_temperature) FROM images;") / 10.0 );
	printf ("<b>Average CCD temperature:</b> %0.1f <br>\n", simple_query ($con, "SELECT avg(img_temperature) FROM images;") / 10.0);
	printf ("<b>Highest CCD temperature:</b> %0.1f <br>\n", simple_query ($con, "SELECT max(img_temperature) FROM images;") / 10.0);
	echo "<b>Images form:</b>" . simple_query ($con, "SELECT min(img_date) FROM images;") . "<br>\n";
	echo "<b>Images to:</b>" . simple_query ($con, "SELECT max(img_date) FROM images;") . "<br>\n";

	$tar_sum = simple_query ($con, "SELECT count(*) FROM targets");

	$tar_noimages_sum = simple_query ($con, "SELECT count(*) FROM targets_noimages");

	echo "<b>Number of fields:</b> $tar_sum<br>\n";

	echo "<b>Fields without image:</b> $tar_noimages_sum (" . round ($tar_noimages_sum / $tar_sum * 100, 2) . "%)<br>\n";

	$obs_lastnight = simple_query ($con, "SELECT count (*) FROM observations WHERE age (current_timestamp (), obs_start) < INTERVAL '1 0:0:0'");

	$obs_succlastnight = simple_query ($con, "SELECT count (*) FROM observations WHERE age (current_timestamp (), obs_start) < INTERVAL '1 0:0:0'
	AND exists (SELECT * FROM images WHERE images.obs_id = observations.obs_id)"); 

	$obs_onlylastnight = simple_query ($con, "SELECT count (*) FROM observations WHERE age (current_timestamp (), obs_start) < INTERVAL '1 0:0:0'
	AND exists (SELECT * FROM images WHERE images.obs_id = observations.obs_id) AND 
	not exists (SELECT * FROM observations as obs1 WHERE observations.tar_id = obs1.tar_id AND 
	age (obs1.obs_start) >= INTERVAL '1 0:0:0')");

	echo "<b>Number of observatiosn within last 24 hours:</b> $obs_lastnight<br>\n";

	echo "<b>From that number of sucessfull (e.g. with at least one image) observations:</b> $obs_succlastnight (" . round ($obs_succlastnight / $obs_lastnight * 100, 2) . "%)<br>\n";

	echo "<b>From that number of new fields:</b> $obs_onlylastnight (" . round ($obs_onlylastnight / $obs_succlastnight * 100, 2) . "%)<br>\n";

	echo "<b>Images by month:</b><br/>";

	$res = pg_query ("SELECT extract (YEAR FROM img_date) as y, extract (MONTH FROM img_date) as m, count (*) as c FROM images GROUP BY extract (YEAR FROM img_date), extract (MONTH FROM img_date) ORDER BY extract (YEAR FROM img_date), extract (MONTH FROM img_date);");

	$last_year = 0;
	$count = 0;

	while ($row = pg_fetch_row ($res))
	{
		if ($last_year != $row[0]) {
			if ($last_year) {
				for ($i = $count; $i < 2; $i++) {
					echo "\t<td collspan='2'></td>";
				}
				echo "</table>\n";
			}
			
			$count = 0;
			$last_year = $row[0];
			echo "<b>$last_year</b><br/>\n<table>\n<tr>\n";
		}

		echo "\t<td style='padding-left:2cm;'><b>$row[1]:</b></td>
\t<td align='right'><a href='bartdb_months.php?month=$row[1]&year=$last_year'>$row[2]</a></td>\n";
		if ($count == 2) {
			echo "</tr><tr>";
			$count = 0;
		}
		else {
			$count ++;
		}
	}

	for ($i = $count; $i < 2; $i++) {
		echo "\t<td collspan='2'></td>";
	}

	echo "</tr>\n</table>\n";

	pg_close ($con);

	konec ();
?>

