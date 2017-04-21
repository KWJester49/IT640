<?php
require_once('../InterfaceLib.inc');

function createGuestCookie()
{
	setcookie('username', 'Guest', time()+60*60*24, "/");
	setcookie('password', NULL, time()+60*60*24, "/");
}

$db= new InventoryAccess();

if(isset($_POST['username']) && isset($_POST['password']))
{
	if($db->databaseLogin($_POST['username'], $_POST['password']))
	{
		setcookie('username', $_POST['username'], time()+60*60*24, "/");
		setcookie('password', $_POST['password'], time()+60*60*24, "/");
	}
	else
	{
		createGuestCookie();
	}
}
else
{
	createGuestCookie();
}
header('Location: index.html');

?>