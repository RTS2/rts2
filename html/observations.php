<?php
#	include "libnova.php";
	include "infos.php";

	hlavicka ("Observations", "Observations details", " ", "ok");
	$q = new Query;
	if (array_key_exists('obs_id', $_SESSION)) {
		$q->add_field ('observations.tar_id, observations.obs_id, observations.obs_start, observations.obs_duration, targets.tar_ra, targets.tar_dec, targets.tar_name, observations_images.img_count');
		$q->add_from ("observations, targets, observations_images");
		$q->add_and_where ("targets.tar_id = observations.tar_id");
		$q->add_and_where ("observations.obs_id = $_SESSION[obs_id]");
		$q->add_and_where ("observations.obs_id = observations_images.obs_id");
		$q->do_query ();
		$q->print_form ();
		$q->clear ();
		$q->add_field ("imgpath (med_id, epoch_id, mount_name, camera_name, images.obs_id, tar_id, img_date) as img_path, img_date, img_exposure/100 as img_exposure_sec, img_temperature/10 as img_temperature_deg, img_filter, imgrange (astrometry) as img_range, images.obs_id, camera_name, mount_name, observations.tar_id");
		$q->from = 'images, observations';
		$q->add_and_where ("observations.obs_id = images.obs_id");
		$q->add_and_where ("observations.obs_id = $_SESSION[obs_id]");
		$q->add_order ('img_date DESC');
		$q->do_query ();
		$q->print_table ();
		$q->clear ();
		$q->add_field ("count_value, count_date, count_exposure, count_filter, counter_name, count_ra, count_dec");
		$q->from = 'targets_counts';
		$q->add_and_where ("obs_id = $_SESSION[obs_id]");
		$q->add_order ('count_date DESC');
		$q->do_query ();
		$q->print_table ();
	} else {
		$q->add_field ('observations.tar_id, observations.obs_id, observations.obs_start, observations.obs_duration, targets.tar_ra, targets.tar_dec, targets.tar_name, targets.type_id, observations_images.img_count');
		$q->add_from ('observations, targets, observations_images');
		if (isset($_REQUEST['night'])) {
			$n = intval ($_REQUEST['night']);
			if ($n) {
				$q->add_and_where ("night_num (obs_start) = $n");
				print_night ($n);
			}
		}
		$q->add_and_where ("targets.tar_id = observations.tar_id");
		$q->add_and_where ("observations.obs_id = observations_images.obs_id");
		if (array_key_exists('type_id', $_REQUEST) AND preg_match('/^[A-Za-z]$/', $_REQUEST['type_id']))
		{
			echo 'Observations for "' . $q->simple_query ("SELECT type_description FROM types WHERE type_id = '$_REQUEST[type_id]'") . '".<br>' . "\n";
			$q->add_and_where ("targets.type_id = '$_REQUEST[type_id]'");
		}
		$q->add_order ('obs_start DESC');
		$q->do_query ();
		$q->print_table ();
	}
	$q->close ();
	konec ();
?>
