<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['item_id']) && !empty($_POST['quantity']))
{
	$db->ADMIN_updateInventoryQuantity($_POST['item_id'], $_POST['quantity']);
	
}
header('Location: inventory.html');

?>