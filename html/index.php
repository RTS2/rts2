<?php
	require_once "fitsdb.php";

	hlavicka ("main page", "entry page", "", "ok");
?>
	<p>Welcome to the RTS2 database main page. Please use menu on right to navigate throught pictures.
	Particulary please fill-in <a href="settings.php">settings</a> for your output customization.</p>
<?php
	if ($_SESSION['authorized'])
		echo "<p>You are logged as '$_SESSION[login]'. You can logout <a href='logout.php'>there</a></p>\n";
	else
		echo "<p>Please <a href='login.php'>login</a>.</p>\n";
	konec ();
?>
