<?php
	include "fn.php";
	include "libnova.php";
	include "simple_query.php";

	hlavicka ("GRB List", "GRB List", " ", "ok");

	function print_hm ($val)
	{
		$val += 0.5;
		if ($val > 1)
		{
			$val --;
		}
		$val *= 24.0;
		$h = floor ($val);
		$m = round(($val - $h) * 60, 0);

		return sprintf ("%02d:%02d", $h, $m);
	}

	$JD = get_julian_from_sys ();
	$st = get_mean_sidereal_time ($JD);
	
	$lng = -14.95;
	$lat = 50;

	$query_string = "SELECT grb.tar_id, grb.grb_id, tar_ra, tar_dec, grb_date, grb_last_update, round (obj_alt(tar_ra, tar_dec, $st, $lng, $lat)) AS alt, 
	round (obj_az (tar_ra, tar_dec, $st, $lng, $lat)) as az FROM targets, grb WHERE
	targets.tar_id = grb.tar_id ORDER BY grb_last_update DESC";
	$con = pg_connect ("dbname=stars");
	$res = pg_query ($con, $query_string);
	echo <<<EOT
<table>
<th>
	<tr>
		<td>tar_id</td>
		<td>grb_id</td>
		<td>RA</td>
		<td>DEC</td>
		<td>Event date</td>
		<td>Last update</td>
		<td>al</td>
		<td>az</td>
		<td>rise</td>
		<td>transit</td>
		<td>set</td>
		<td>images #</td>
	</tr>
</th>
<tb>
EOT;
	while ($row = pg_fetch_row ($res)) {
		$img_c = simple_query ($con, "SELECT img_count FROM targets_imgcount WHERE tar_id = " . $row[0] . ";");

		echo "<tr " . ($img_c > 0 ? "bgcolor='lightgray'" : "") . "><td>" . join ("</td>\n<td>", $row) . "</td>\n";
		$ra = $row[2];
		$dec = $row[3];
		$r = get_object_rise ($JD,  $lng, $lat, $ra, $dec);
		$r -= floor ($r);
		$t = get_object_transit ($JD,  $lng, $lat, $ra, $dec);
		$t -= floor ($t);
		$s = get_object_set ($JD,  $lng, $lat, $ra, $dec);
		$s -= floor ($s);

		echo "<td>" . print_hm ($r) . "</td>\n";
		echo "<td>" . print_hm ($t) . "</td>\n";
		echo "<td>" . print_hm ($s) . "</td>\n"; 
		echo "<td>$img_c</td>\n";
		echo "</tr>\n";
	}

	echo "</tb></table>";
?>
