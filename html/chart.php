<?
	$ra = escapeshellarg (round($_REQUEST["ra"]/15.0,2));
	$dec = escapeshellarg (round($_REQUEST["dec"],2));
	$cmd = "cd /home/petr/starchart && echo 'hoj' && ./starpng -b -m 12 -s 40.0 -r $ra -d $dec -l 12";

	$ret = popen ("$cmd", "r");
	$bit = fread ($ret, 1000);
	if ($bit > "")
	{
		header ('Content-type: image/png');
		
		echo $bit;
		while ($bit = fread ($ret, 1000))
		{
			echo $bit;
		}
	} else {
		echo "Error in popen! Failed command was '$cmd'<br/>\nChyba v popen! Selhal pøíkaz $cmd.";	
	}
	pclose ($ret);
?>
