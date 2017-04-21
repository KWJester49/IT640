<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['receivableOrder']))
{
	foreach($_POST['receivableOrder'] as $combinedOrderItem)
	{	
		$order_id= substr($combinedOrderItem, 0, 14);
		$item_id= substr($combinedOrderItem, 14, 13);
		$db->receiveOrder($order_id, $item_id);
	}
}
header('Location: orders.html');

?>