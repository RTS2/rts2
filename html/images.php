<?php
	require_once "fitsdb.php";

	hlavicka ("Images", "Images", " ", "ok");


function image_form ()
{
	$img_id = intval ($_REQUEST['img_id']);

	$q = new Query;

	$q->add_field ("imgpath (med_id, epoch_id, mount_name, camera_name, images.obs_id, tar_id, img_date) as img_path, img_date, round(img_exposure/100.0,2) as img_exposure_sec, round(img_temperature/10.0,2) as img_temperature_deg, img_filter, imgrange (astrometry) as img_range, images.obs_id, observations.tar_id, camera_name");
	$q->add_from ("images, observations");
	$q->add_and_where ("observations.obs_id = images.obs_id");
	$q->add_and_where ('img_id = ' . $img_id);
	$q->do_query ();
	$q->print_table ();

	if ($_SESSION['authorized'])
	{
		if (array_key_exists ('confirmation', $_REQUEST))
		{
			echo "Deleting file $img_id.";
			$q->do_query ('update images set delete_flag = true where img_id = ' . $img_id );
echo <<<EOF
<form action='images.php'>
	<input type='hidden' name='img_id' value='$img_id'/>
	<input type='submit' value='Back' name='form'/>
</form>
EOF;
		}
		else
		{	
			echo <<<EOF
Are you sure to delete image $img_id? <br>
<form action='images.php'>
	<input type='hidden' name='img_id' value='$img_id'/>
	<input type='submit' value='Yes' name='confirmation'/>
	<input type='submit' value='No way!' name='form'/>
</form>
EOF;
		}
	}
	else
	{
		echo 'Please login, you aren\'t authorized.';
	}
}

function image_table ()
{
	echo "<form action='images.php'>\n<table>\n";
		display_settings ('ra');
		display_settings ('dec');
		display_settings ('date_from');
		display_settings ('date_to', 1);
		display_settings ('camera_name');
		display_settings ('mount_name');
		display_settings ('tar_id');
	echo "<td><input type='submit' value='Change'></input></td></tr></table>\n</form>";


	$q = new Query;

	$q->add_field ('img_id');
	$q->add_field ("imgpath (med_id, epoch_id, mount_name, camera_name, images.obs_id, tar_id, img_date) as img_path, img_date, round(img_exposure/100.0,2) as img_exposure_sec, round(img_temperature/10.0,2) as img_temperature_deg, img_filter, imgrange (astrometry) as img_range, images.obs_id, observations.tar_id, camera_name");
	$q->add_from ("images, observations");
	$q->add_and_where ("observations.obs_id = images.obs_id");
	$q->add_and_test_where ("img_date >= ", 'date_from');
	$q->add_and_test_where ("img_date <= ", 'date_to');
	$q->add_and_test_where ("camera_name = ", 'camera_name');
	$q->add_and_test_where ("mount_name = ", 'mount_name');
	$q->add_and_test_where ('observations.tar_id = ', 'tar_id');
	$q->add_and_test_where ("images.obs_id = ", 'obs_id');
	if (array_key_exists('ra', $_SESSION) && array_key_exists('dec', $_SESSION)) 
		$q->add_and_where ("isinwcs ($_SESSION[ra], $_SESSION[dec], astrometry)");
	if (isset ($_REQUEST['night'])) {
		$n = intval ($_REQUEST['night']);
		if ($n) {
			$q->add_and_where ("night_num (img_date) = $n");
			print_night ($n);
		}
	}
	$q->add_order ("img_date DESC");
	$q->do_query ();
	$q->print_table ();
	$q->close ();
}

	if (array_key_exists ('img_id', $_REQUEST) && !array_key_exists ('form', $_REQUEST))
	{
		image_form ();
	}
	else
	{
		image_table ();
	}
	
	konec ();
?>
