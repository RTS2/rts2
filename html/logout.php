<?php
	require_once "fitsdb.php";

	hlavicka ("logout", "logout", "", "ok");
	unset($_SESSION['authorized']);
	unset($_SESSION['login']);
?>
	<p>You have been logged out of BARTDB.</p>
<?php
	konec ();
?>
