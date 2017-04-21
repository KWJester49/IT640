<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['item_id']))
{
	$num_length = strlen((string)$_POST['item_id']);
	if($num_length==13)
	{
		$db->ADMIN_deleteItem($_POST['item_id']);
	}
}
header('Location: inventory.html');

?>