<?
	include "fn.php";
	include "fitsdb.php";

	hlavicka ("BARTDB", "Ra &amp; dec settings", " ", "ok");
?>

<form action="images.php" method="post">
	<table>
	<tr><td align="right">
		Ra:
	</td><td>
		<input type="text" name="ra" size="10" value="180.0"></input>
	</td></tr><tr><td align="right">
		Dec:
	</td><td>
		<input type="text" name="dec" size="10" value="45.0"></input>
	</td></tr>
	</table>
	<input type="submit" value="Set"></input>
</form>

<?
	konec ();
?>
