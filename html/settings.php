<?
	require_once "fitsdb.php";

	hlavicka ("BARTDB Settings", "BARTDB Settings", "", "ok");

?>
	<form action="settings.php" method="get">

	<table>
<?
		echo "<tr>";
		display_settings ('ra');
		echo "</tr><tr>";
		display_settings ('dec');
		echo "</tr><tr>";
		display_settings ('tar_id');
		echo "</tr><tr>";
		display_settings ('obs_id');
		echo "</tr><tr>";
		display_settings ('date_from');
		echo "</tr><tr>";
		display_settings ('date_to');
		echo "</tr><tr>";
		display_settings ('page_size');
		echo "</tr><tr>";
#		display_mounts ('mount');
#		echo "</tr>";
?>
	</table>
	<input type="submit" value="Submit"></input>
	</form>
<?
	konec ();
?>
