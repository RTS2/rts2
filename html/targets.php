<?
	require_once "fitsdb.php";
	include "infos.php";

	hlavicka ("BARTDB", "Targets details", " ", "ok");

	$q = new Query;
	$q->connect ("dbname=stars");

	$targets_type_from = array ( 
		"S" => "",
		"O" => "ot",
		"G" => "grb"
	);

	if ($_SESSION['authorized'] && array_key_exists('tar_id', $_REQUEST) && array_key_exists('type_id', $_REQUEST)) {
		$tar_id = intval ($_REQUEST['tar_id']);
		if (!preg_match ('/^[A-Za-z]$/', $_REQUEST['type_id']))
		{
			echo "Invalid type_id entered. Please return back and correct that";
			exit (1);
		}
		$type_id = $_REQUEST['type_id'];
		$tar_name = $_REQUEST['tar_name'];
		preg_replace ('/[^A-Za-z0-9 _\-.]/', '', $tar_name);
		$tar_ra = s2deg($_REQUEST['tar_ra'], 15);
		$tar_dec = s2deg($_REQUEST['tar_dec'], 1);
		$tar_comment = $_REQUEST['tar_comment'];
		preg_replace ('/[^A-Za-z0-9 _.\-]/', '', $tar_comment);
		if (array_key_exists('insert',$_REQUEST))
		{
			$q->do_query ("INSERT INTO targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment) VALUES (nextval('tar_id'), '$type_id', '$tar_name', $tar_ra, $tar_dec, '$tar_comment');");
			echo "INSERT INTO targets (tar_id, type_id, tar_name, tar_ra, tar_dec, tar_comment) VALUES (nextval('tar_id'), '$type_id', '$tar_name', $tar_ra, $tar_dec, '$tar_comment');";
		}
		else
			$q->do_query ("UPDATE targets SET type_id='$type_id', tar_name = '$tar_name', tar_ra = $tar_ra, tar_dec = $tar_dec, tar_comment = '$tar_comment' WHERE tar_id = $tar_id");
	}

	if (array_key_exists('tar_id', $_SESSION) && !array_key_exists('type_id', $_REQUEST)) {
		$target_type = $q->simple_query ("SELECT type_id FROM targets WHERE tar_id = $_SESSION[tar_id]");
		$q->add_field ("*");
		$q->add_from ('targets');
		$q->add_and_where ("targets.tar_id = $_SESSION[tar_id]");
		if ($targets_type_from[$target_type]) {
			$q->add_from ($targets_type_from[$target_type]);
			$q->add_and_where ("targets.tar_id = $targets_type_from[$target_type].tar_id");
		}
		$q->do_query ();
		$q->print_form ();

		$row = pg_fetch_array ($q->res, 0);
		echo "<img src='chart.php?ra=" . $row['tar_ra'] . "&dec=" . $row['tar_dec'] . "' alt='Sky chart'/>\n<hr>\n";
		$img_count = $q->simple_query ("SELECT img_count FROM targets_images WHERE tar_id = $_SESSION[tar_id]");
		echo "Images: <a href='images.php'>$img_count</a>";
		
		if ($img_count > 0) {
			$last_image = $q->simple_query ("SELECT MAX(img_date) FROM images, observations WHERE observations.obs_id = images.obs_id AND observations.tar_id = $_SESSION[tar_id]");
			echo "<hr>\nLast image ($last_image):<br><img src='preview.php?fn=" . $q->simple_query ("SELECT imgpath (med_id, epoch_id, mount_name, camera_name, images.obs_id, tar_id, img_date) FROM images, observations WHERE images.obs_id = observations.obs_id AND observations.tar_id = $_SESSION[tar_id] AND img_date = '$last_image'") . "&full=true'/>\n<p>\n";
		}
		$q->clear ();
		$q->add_field ('observations.obs_id, observations.obs_start, observations.obs_duration, observations_images.img_count');
		$q->add_from ('observations, observations_images');
		$q->add_and_where ("observations.tar_id = $_SESSION[tar_id]");
		$q->add_and_where ("observations_images.obs_id = observations.obs_id");
		$q->add_order ('obs_start DESC');
		$q->do_query ();
		$q->print_table ();
	} else if (array_key_exists ('insert', $_REQUEST)) {
		$q->add_field ("*");
		$q->add_from ("targets");
		$q->do_query ();
		$q->print_form_insert ();
	} else {
		$q->add_field ('targets.*, targets_images.img_count');
		$q->add_from ('targets, targets_images');
		if (array_key_exists('type_id', $_REQUEST)) {
			$type_id = $_REQUEST['type_id'];
			if (preg_match('/^[A-Za-z]$/', $type_id))
				$q->add_and_where ("type_id = '$type_id'");
			switch ($type_id) {
				case 'G':
					$q->add_field ('grb.*');
					$q->add_from ('grb');
					$q->add_and_where ('grb.tar_id = targets.tar_id');
					$q->add_order ('grb_id DESC');
					break;
				case 'O':
					$q->add_field ('ot.*');
					$q->add_from ('ot');
					$q->add_and_where ('ot.tar_id = targets.tar_id');
					break;
			}
		}
		$q->add_and_where ('targets.tar_id = targets_images.tar_id');
		$q->add_order ('img_count DESC');
		$q->do_query ();
		$q->print_table();
	}
	$q->close ();	
	konec ();
?>
