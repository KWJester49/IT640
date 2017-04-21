<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['usedItem']) && !empty($_POST['quantity']))
{
	$db->useItem($_POST['usedItem'], $_POST['quantity']);
}
header('Location: inventory.html');

?>