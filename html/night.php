<?
	require_once "fitsdb.php";
	hlavicka ("NIGHT Statistics", "", "", "ok");
	$n = intval ($_REQUEST['night']);
	echo "<h2>Night statistics</h2>";
	print_night ($n);
	$q = new Query;
	$q->connect("dbname=stars");
	$q->fields = "MIN(img_date), MAX(img_date), MIN(img_temperature), AVG(img_temperature), MAX(img_temperature), count(*)";
	$q->from = "images";
	$q->where = " WHERE night_num(img_date) = $n";
	$res = $q->do_query ();
	if (($row = pg_fetch_row ($res)) and ($row[5] > 0)) {
		echo "Total of <a href='images.php?night=$n'>$row[5]</a> <b>images</b> were taken from $row[0] to $row[1]<br>\n";
		printf ("<b>CCD Temperatures</b> - min: %+.2f avg: %+.2f max: %+.2f<br>\n", $row[2]/10.0, $row[3]/10.0, $row[4]/10.0);
		$q->fields = "camera_name,count(*), MIN(img_temperature), AVG(img_temperature), MAX(img_temperature), MIN(img_date), MAX(img_date), EXTRACT (EPOCH FROM MAX(img_date) - MIN(img_date))";
		$q->where = " WHERE night_num(img_date) = $n GROUP BY camera_name";
		$res = $q->do_query ();
		echo "Number of images and temperature statistics per camera:\n<table>
<th><tr><td><b>Name</b></td><td>#images</td><td>min t(&deg;C)</td><td>avg t (&deg;C)</td><td>max t(&deg;C)</td><td>from</td><td>to</td><td>duration (sec)</td><td>avg. image time (sec)</td></tr></th>\n<tb>";
		while ($row = pg_fetch_row ($res))
			printf ("\t<tr><td><b>%s</b></td><td><a href='images.php?night=$n&camera_name=$row[0]'>%d</a></td><td>%+.2f</td><td>%+.2f</td><td>%+.2f</td><td>$row[5]</td><td>$row[6]</td><td>$row[7]</td><td>%0.3f</td>\n", $row[0], $row[1], $row[2]/10.0, $row[3]/10.0, $row[4]/10.0, $row[7] / $row[1]);
		echo "</tb>\n</table>\n";
	} else {
		echo "Cannot find any image.<br>\n";
	}
	$q->fields = "count(*),MIN(obs_start), MAX(obs_start + obs_duration), MIN(obs_duration), AVG(obs_duration), MAX(obs_duration), EXTRACT (EPOCH FROM SUM(obs_duration)), EXTRACT (EPOCH FROM MAX(obs_start + obs_duration) - MIN (obs_start))";
	$q->from = "observations";
	$q->where = " WHERE night_num(obs_start) = $n";
	$res = $q->do_query ();
	if (($row = pg_fetch_row ($res)) and ($row[0] > 0)) {
		echo "Total of <a href='observations.php?night=$n'>$row[0]</a> <b>observations</b> from $row[1] to $row[2]<br>\n";
			echo "<b>Observations duration</b> - min: $row[3] avg: $row[4] max: $row[5]<br>\n";
			printf ("<b>Total observation time</b> $row[6] out of $row[7] sec, %.2f %% of time used<br>\n", 100 * $row[6]/$row[7]);
			printf ("<hr><b>List of all observations</b><br>\n");
			$q->clear ();
			$q->add_field ('observations.tar_id, observations.obs_id, observations.obs_start, observations.obs_duration, targets.tar_ra, targets.tar_dec, targets.tar_name, targets.type_id, observations_images.img_count');
			$q->add_from ('observations, targets, observations_images');
			$q->add_and_where ("night_num (obs_start) = $n");
			$q->add_and_where ("targets.tar_id = observations.tar_id");
			$q->add_and_where ("observations.obs_id = observations_images.obs_id");
			$q->add_order ('obs_start DESC');
			$q->do_query ();
			$q->print_table ();
		} else {
			echo "Cannot find any observation.<br>\n";
		}
	$q->close ();
	konec ();
?>
