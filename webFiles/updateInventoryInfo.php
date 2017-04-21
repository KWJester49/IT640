<?php
require_once('../InterfaceLib.inc');

$db= new InventoryAccess();
$db->databaseLogin($_COOKIE['username'], $_COOKIE['password']);

if(!empty($_POST['item_id']))
{
	$item_id= $_POST['item_id'];
	$info= $db->ADMIN_getUpdatableInventoryInfoForItemID($_POST['item_id']);
	$quantity= $info['quantity'];
	$location= $info['location'];
	$par_level= $info['par_level'];
	$on_order= $info['on_order'];
	
	if(!empty($_POST['quantity']))
	{
		$quantity=$_POST['quantity'];
	}
	if(!empty($_POST['location']))
	{
		$location=$_POST['location'];
	}
	if(!empty($_POST['par_level']))
	{
		$par_level=$_POST['par_level'];
	}
	if(!empty($_POST['on_order']) && $_POST['on_order'] !="NOT SELECTED")
	{
		if($_POST['on_order']=="FALSE")
		{
			$on_order=0;	
		}
		else
		{
			$on_order=1;
		}
		
	}

	$db->ADMIN_updateInventoryInfo($item_id, $quantity, $location, $par_level, $on_order);
	
}
header('Location: inventory.html');

?>