<?php
require_once('../InterfaceLib.inc');

function createGuestCookie()
{
	setcookie('username', 'Guest', time()+60*60*24, "/");
	setcookie('password', NULL, time()+60*60*24, "/");
}

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

$db->uploadOrder($_FILES['uploadFile']['tmp_name']);
header('Location: orders.html');

?>