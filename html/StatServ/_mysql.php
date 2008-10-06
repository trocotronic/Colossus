<?
//////////////////////
// Tareas de MySQL //
////////////////////

include_once("sql.inc");
if (($db = @mysql_pconnect($db_host, $db_login, $db_pass)))
{
	if (!(@mysql_select_db($db_name, $db)))
	{
		echo "Ha sido imposible seleccionar la base de datos <b>" . $db_name . "</b><br />" . mysql_error();
		exit(0);
	}
}
else
{
	echo "Ha sido imposible conectar con <b>" . $db_host . "</b>. Login: <b>" . $db_login . "</b><br />" . mysql_error();
	exit(0);
}
?>
