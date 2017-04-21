<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['order_id']))
{
	$db->ADMIN_deleteOrder($_POST['order_id']);
}
header('Location: orders.html');

?>