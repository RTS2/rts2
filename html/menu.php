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
	<li><a href="targets.php?tar_id=&obs_id=">Targets</a>
		<ul class="menu">
			<li>
EOT;
	$q = new Query ();
	$res = $q->do_query ("SELECT type_id, type_description FROM types ORDER BY type_id ASC");
	$types = array ();
	while ($row = pg_fetch_row ($res)) {
		array_push ($types, $row[0]); 
		echo "<a href='targets.php?type_id=$row[0]'>$row[0]</a>&nbsp;";
	}
echo "</li>\n";
if ($_SESSION['authorized'])
echo "			<li><a href='targets.php?insert=1'>Insert new</a></li>\n";
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
			<li>Targets</li>
				<ul class="menu">
					<li>
EOT;
	while ($typ = array_shift ($types))
		echo "<a href='observations.php?type_id=$typ'>$typ</a>&nbsp;";
echo <<<EOT
</li>
				</ul>
		</ul class="menu">
	</li>
	<li><a href="days.php?year=2003">Year 2003</a></li>
	<li><a href="types.php">Observations types</a></li>
	<li><a href="medias.php">Medias table</a></li>
EOT;
if ($_SESSION['authorized'])
echo <<<EOT
	<ul class="menu">
		<li><a href="medias.php?insert=1">Insert new</a></li>
	</ul>
EOT;
echo <<<EOT
	<li><a href="settings.php">Settings</a></li>
	<li>
EOT;
	if ($_SESSION['authorized'])
		echo "<a href='logout.php'>Logout</a>";
	else
		echo "<a href='login.php'>Login</a>";
	$q->close ();
echo <<<EOF
</li>
</ul>

</td><td valign="top">
EOF;
}
?>
