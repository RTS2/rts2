<?
	include "fitsdb.php";
	if (!$_SESSION['authorized']) {
		hlavicka ("Bad login", "Bad login", '', "error");
		echo "You aren't authorized to download images.";
		konec ();
		exit (1);
	} 
	$val = '';
	$count = 0;
	foreach (array_keys($_REQUEST) as $key)
	{
		if ($_REQUEST[$key] == 'on' && preg_match ('#^/images/[A-Za-z0-9/\-]*.fits$#', $key))
		{	
			$key = preg_replace ('#^/images/#', '', $key);
			$val .= ' ' . str_replace ('_', '.', $key);
			$count++;	
		}
	}
	if ($count > 10)
	{
		hlavicka ("More then 10 images requested", "More then 10 images", "", "ok");
		echo "Please select only up to 10 images. We have to
create some temporary files to pack your images, so we
cannot ship more then 10 images on one request. When
you need more data, please don't hesitate to contact
	us. <p> <a href='mailto:rtteam@lascaux.asu.cas.cz>rtteam@lascaux.asu.cas.cz</a>";
		konec ();
		exit (1);
	}
	
	$tmpfile = tempnam ('/tmp', 'BARTDB_DWNL');
	if (`cd /images/ && tar -czf $tmpfile $val`)
	{
		echo "Error at system call, please contact <a href='mailto:petr@lascaux.asu.cas.cz?Subject=BARTDB_DOWNLOAD'>administrator</a>";
		exit (1);
	}
	header ('Content-type: application/octet-stream');
	header ('Content-Disposition: attachment; filename="bartdb_images.tar.gz"');
	header ('Content-length: ' . filesize ($tmpfile));
	header("Content-Transfer-Encoding: binary");
	readfile ($tmpfile);
	unlink ($tmpfile);
?>
