<?
	require_once "fitsdb.php";

	function head ($title) {
echo <<<EOT
<html>
<head>
	<link rel="StyleSheet" href="/css/styl.css" type="text/css"/>
	<style type="text/css">
		ul.menu {
			list-style-type: none;
			padding-left: 0.4cm;
		}
	</style>
	<title>$title</title>
</head>
<body>

<table width='100%'>
<tb><tr>
<td valign='top' style='background: #F5F5F5' width='20%'>

<ul class="menu">
	<li>Images
		<ul class="menu">
			<li><a href="images.php">RA&amp;DEC</a></li>
			<li><a href="images.php?ra=&dec=">Target</a></li>
			<li>Night</li>
			<li>Camera
			<li>Mount
		</ul class="menu">
	</li>
	<li><a href="targets.php">Targets</a>
		<ul class="menu">
EOT;
	$con = pg_connect ("dbname=stars");
	$res = pg_query ("SELECT type_id, type_description FROM types");
	while ($row = pg_fetch_row ($res)) {
		echo "\t\t\t<li><a href='targets.php?type_id=$row[0]'>$row[1]</a></li>\n";
	}
	pg_close ($con);
echo <<<EOT
		</ul class="menu">
	</li>
	<li>Statistics
		<ul class="menu">
			<li><a href="statistics_gen.php">General</a></li>
			<li>Telescope load</li>
			<li>CCD temperatures</li>
			<li>Year statistics</li>
		</ul class="menu">
	</li>
	<li><a href="observations.php?obs_id=&tar_id=">Observations</a></li>
		<ul class="menu">
			<li>RA&amp;DEC</li>
			<li>Target</li>
			<li>Night</li>
		</ul class="menu">
	</li>
	<li><a href="days.php?year=2003">Year 2003</a></li>
	<li><a href="types.php">Observations types</a></li>
	<li><a href="settings.php">Settings</a></li>
	<li>
EOT;
#	pg_close ();
	if ($_SESSION['authorized'])
		echo "<a href='logout.php'>Logout</a>";
	else
		echo "<a href='login.php'>Login</a>";
echo <<<EOF
</li>
</ul>

</td><td valign="top">
EOF;
}
?>
