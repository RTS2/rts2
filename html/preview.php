<?
	include "fitsdb.php";

	$fn = escapeshellarg ("$_REQUEST[fn]");
	$ra = "";
	$dec = "";
	if (array_key_exists('full', $_REQUEST))
		$cmd = "/home/rtopera/src/f2cj $fn full";
	else if (array_key_exists('ra', $_SESSION) && array_key_exists('dec', $_SESSION))
		$cmd = "/home/rtopera/src/f2cj $fn $_SESSION[ra] $_SESSION[dec]";
	else
		$cmd = "/home/rtopera/src/f2cj $fn";

	$ret = popen ("$cmd", "r");
	$bit = fread ($ret, 1000);
	if ($bit > "")
	{
		header ('Content-type: image/jpeg');
		
		echo $bit;
		while ($bit = fread ($ret, 1000))
		{
			echo $bit;
		}
	} else {
		echo "Error in popen! Failed command was $cmd<br/>\nChyba v popen! Selhal pøíkaz $cmd.";	
	}
	pclose ($ret);
?>
