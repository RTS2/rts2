<?php
	include "fn.php";
	hlavicka ("BARTDB", "Seznam GRB", " ", "ok");

	$query_string = "SELECT * FROM targets
	ORDER BY tar_id";
	$con = pg_connect ("dbname=stars");
	$res = pg_query ($con, $query_string);
	echo "<table>";

	while ($row = pg_fetch_row ($res)) {
		echo "<tr><td>" . join ("</td><td>", $row) . "</tr></td>\n";
	}

	echo "</table>";
	echo "arr: " . $jazyk . "a";
?>
