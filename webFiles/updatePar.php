<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['usedItem']) && !empty($_POST['par_level']))
{
	$db->updateParLevel($_POST['usedItem'], $_POST['par_level']);
}
header('Location: inventory.html');

?>