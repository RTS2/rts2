<?php
#	include "libnova.php";
	require_once "fitsdb.php";

	function target_info ($con, $tar_id) {
		$res = pg_query ($con, "SELECT tar_name, tar_ra, tar_dec, targets.type_id, type_description FROM targets, types WHERE tar_id = $tar_id AND targets.type_id = types.type_id");
		if ($row = pg_fetch_row ($res)) {
			echo "\t<table>\n";
			echo "\t\t<tr><td align='right'>Target id:</td><td>$tar_id</td></tr>\n";
			echo "\t\t<tr><td align='right'>Name:</td><td>" . $row[0] . "</td></tr>\n";
			echo "\t\t<tr><td align='right'>Type:</td><td>" . $row[3] . "-" . $row[4] . "</td></tr>\n";
			echo "\t\t<tr><td align='right'>RA:</td><td>" . $row[1] . "</td></tr>\n";
			echo "\t\t<tr><td align='right'>DEC:</td><td>" . $row[2] . "</td></tr>\n";
			$jd = get_julian_from_sys ();
			echo "\t\t<tr><td align='right'>Rise:</td><td>" . get_object_rise ($jd, $_SESSION['long'], $_SESSION['lat'], $row[1], $row[2]) . "</td></tr>\n"; 
#			echo "\t\t<tr><td align='right'>At 10 deg:</td><td>" . get_object_rise_horizont ($jd, -14, 50, $row[1], $row[2], 10) . "</td></tr>\n"; 
			echo "\t</table>\n";
			echo "<img src='chart.php?ra=".$row[1]."&dec=".$row[2]."'>\n";
			return 0;
		} else {
			echo "No such target $tar_id.\n";
			return -1;
		}
	}

	function target_obs_info ($con, $tar_id) {
		$res = pg_query ($con, "SELECT obs_id, obs_start, obs_duration FROM observations WHERE tar_id = $tar_id ORDER BY obs_start DESC");

		if (pg_num_rows ($res)) {
			echo "<table>\n";
			echo "<th><tr><td>obs</td><td>start</td><td>duration</td></tr></th>\n";
			while ($row = pg_fetch_row ($res)) {
				$obs_id = array_shift ($row);
				echo "\t<tr>\n\t\t<td bgcolor='lightgray'><a href='observation.php?obs_id=$obs_id'>$obs_id</a></td>\n\t\t<td bgcolor='lightgray'>" . join ("</td>\n\t\t<td bgcolor='lightgray'>", $row) . "</td>\n\t</tr>\n";
			}
			echo "</table>\n";

		} else {
			echo "No observations for target id $tar_id.\n";
		}
	}
?>
