<?php
	require_once "fitsdb.php";
	$q = new Query;
	hlavicka ("BOOTES DB", "Medias", " ", "ok");
	if ($_SESSION['authorized'] && array_key_exists ('med_path', $_REQUEST)) {
		$med_id = -1;
		if (array_key_exists ('med_id', $_REQUEST))
			$med_id = intval ($_REQUEST['med_id']);
		$med_path = $_REQUEST['med_path'];
		preg_replace ('/[^A-Za-z0-9_\-.]/', '', $med_path);
		if (array_key_exists('insert',$_REQUEST))
			$q->do_query ("INSERT INTO medias VALUES (nextval('med_id'), '$med_path');");
		else
			$q->do_query ("UPDATE medias SET med_path = '$med_path' WHERE med_id = $med_id");
		$q->add_field ("*");
		$q->add_from ("medias");
		$q->add_order ("med_id ASC");
		$q->do_query ();
		$q->print_table ();
	} else if (array_key_exists ('insert', $_REQUEST)) {
		$q->add_field ("*");
		$q->add_from ("medias");
		$q->do_query ();
		$q->print_form_insert ();
	} else if (array_key_exists ('med_id', $_REQUEST)) {
		$q->add_field ("*");
		$q->add_from ("medias");
		$q->add_and_where ("med_id = " . intval ($_REQUEST['med_id']));
		$q->do_query ();
		$q->print_form ();
	} else {
		$q->add_field ("*");
		$q->add_from ("medias");
		$q->add_order ("med_id ASC");
		$q->do_query ();
		$q->print_table ();
	}
	$q->close ();
	konec ();
?>
