<?php
	include "fn.php";
	hlavicka ("Results", "Querry results..", " ", "ok");

	function s2deg ($s) {
		$i = 0;
		$len = strlen ($s);
		$res = '';
		for ($i = 0; $i < $len && ($c = $s[$i]) != ':'; $i++)
			$res .= $c;
		if ($i == $len)
			return $res;
		
		$out = 15 * $s;
		$mul = $out < 0 ? -0.25 : 0.25;
		$res = '';
		for ($i++; $i < $len; $i++) {
			$c = $s[$i];
			if ($c == ':') {
				$out += abs($res) * $mul;
				$mul *= 0.25;
				$res = '';
			}
			else
				$res .= $c;
		}
		return $out + abs($res) * $mul;
	}
	
	function process () {

		$month = $_REQUEST[month];
		$year = $_REQUEST[year];


		if (($month < 1) or ($month > 13))
		{
			echo "<p>Invalid month selected</p>
			<p>Back to <a href='index.php'>query page</a></p>";
			return;
		}

		if (($year < 1997) or ($year > 2003))
		{
			echo "<p>Invalid year selected</p>
			<p>Back to <a href='index.php'>query page</a></p>";
			return;
		}


		$query_string = "SELECT img_name, img_date, img_exposure/100, img_temperature/10, img_filter, imgrange (astrometry) FROM images WHERE DATE_PART ('year', img_date) = $year AND DATE_PART ('month', img_date) = $month ORDER BY img_date;";

		$con = pg_connect ("dbname=stars");
	
		$res = pg_query ($con, $query_string);

		if (pg_numrows ($res) == 0) {
			echo "<p>There are no images available from $_REQUEST[poz_od] to $_REQUEST[poz_do] for RA $ra_deg, DEC $dec_deg in our database.</p>
			<p>Back to <a href='index.php'>query page</a></p>";
			
			return;
		}
	
		echo "<b>RA:</b> $ra_deg <b>DEC:</b> $dec_deg (J2000)<br>\n";

		echo "<b>From:</b> $_REQUEST[poz_od] <b>To:</b> $_REQUEST[poz_do] <br>\n";

		echo "<b>Matching images:</b> " . pg_numrows ($res) . "<br>

<form action='download.php' method='post'>
<table style='border: 1pt solid black; background: #dfdfdf' cellpadding='3' cellspacing='0'>	<th>
		<tr>
			<td style='background: grey'>D</td>
			<td style='background: white'>image filename</td>
			<td style='background: white'>date of observation</td>
			<td style='background: white'>exp (sec) </td>
			<td style='background: white'>t (&deg;C)</td>
			<td style='background: white'>filter</td>
			<td style='background: white'>field <br>ra-dec ra-dec</td>
		</tr>
	</th>
";
		
		while ($row = pg_fetch_row ($res)) {
			echo "\t<tr>\n\t\t<td style='border-right: 1pt dotted darkred;'><input type='checkbox' name='$row[0]' checked></input>";
			$last = array_pop ($row);
			foreach ($row as $item)
				echo "\t\t<td style='border-right: 1pt solid darkblue;'>$item</td>\n";
			echo "\t\t<td>$last</td>\n";
			echo "\t</tr>";
		}

		echo "</table>

<hr width='0'/>
<table>
	<tr>
		<td align='right'>Login:</td>
		<td align='left'><input type='text' name='login' size=10></input></td>
	</tr><tr>
		<td align='right'>Password:</td>
		<td align='left'><input type='password' name='passwd' size=10></input></td>
	</tr>
</table>
<input type='submit' value='Download'></input>
<input type='reset' value='Reset'></input>
</form>";
		pg_close ();
	}
	

	// overeni parametru
	$datum_match = "/^\s*\d{2,4}-\d{1,2}-\d{1,2}\s*$/";
	if (! (preg_match ($datum_match, $_REQUEST[poz_od]) and preg_match ($datum_match, $_REQUEST[poz_do]))) {
		// spatne zadane datum, nahlas to a skonci, at se to pak nedela neplechu v dotazu..
		echo "Bad date format, please give date at 'YYYY-MM-DD' format.</br>";
	}
	else {
		process ();
	}
	konec ();
?>
