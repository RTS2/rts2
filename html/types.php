<?
	require_once "fitsdb.php";
	hlavicka ("BARTDB", "Types details", " ", "ok");
	$q = new Query;
	$q->connect ("dbname=stars");
	if (array_key_exists('type_description', $_REQUEST) && array_key_exists('type_id', $_REQUEST) && preg_match ('/^[A-Za-z]$/', $_REQUEST['type_id'])) {
		$type_description = $_REQUEST['type_description'];
		preg_replace ('/[^A-Za-z0-9 _\-.]/', '', $type_description);
		if (array_key_exists ('insert', $_REQUEST))
		{
			$q->simple_query ("INSERT INTO types (type_description, type_id) VALUES ('$type_description', '$_REQUEST[type_id]');");
		}
		else 
			$q->simple_query ("UPDATE types SET type_description = '$type_description' WHERE type_id = '$_REQUEST[type_id]'");
	}

	if (array_key_exists('type_id', $_REQUEST) && preg_match ("/^[A-Za-z]$/", $_REQUEST['type_id'])) {
		$q->add_field ("*");
		$q->add_from ('types');
		$q->add_and_where ("type_id = '$_REQUEST[type_id]'");
		$q->do_query ();
		$q->print_form ();
	} else if (array_key_exists ('insert', $_REQUEST)) {
		$q->add_field ('*');
		$q->add_from ('types');
		$q->do_query ();
		$q->print_form_insert();
	} else {
		$q->add_field ('*');
		$q->add_from ('types');
		$q->add_order ('type_id');
		$q->do_query ();
		$q->print_table();
	}
	$q->close ();

	konec ();
?>
