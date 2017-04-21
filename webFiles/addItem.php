<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['item_id']) && !empty($_POST['item_name']) && !empty($_POST['item_desc']))
{
	$num_length = strlen((string)$_POST['item_id']);
	if($num_length==13)
	{
		$db->ADMIN_addItem($_POST['item_id'], $_POST['item_name'], $_POST['item_desc']);
	}
}
header('Location: inventory.html');

?>