<?
	include "fn.php";
	hlavicka ("BARTDB", "Seznam GRB", " ", "ok");

	$query_string = "SELECT targets.*, targets_images.img_count FROM targets, targets_images WHERE targets.tar_id = targets_images.tar_id ORDER BY targets_images.img_count DESC";
	$con = pg_connect ("dbname=stars");
	$res = pg_query ($con, $query_string);
	echo "<table>";

	while ($row = pg_fetch_row ($res)) {
		echo "<tr><td>" . join ("</td><td>", $row) . "</tr></td>";
	}

	echo "</table>";
?>
