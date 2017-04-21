<?php

class InventoryAccess
{
	
	private $db;
	private $access_level;
	const GUEST=0;//Low level access of reports, cannot change anything in database
	const USER=3;//General user of the database and supply storeroom. Above plus can receive items, submit order files, and use items.
	const ADMIN=5;//Can do all of the above plus can manually change data (data stewarding) and create other users.

	public function __construct()
	{
		$this->db = new mysqli("localhost","root","12345","MainORDB");
		if ($this->db->connect_error)
		{
			echo "error connecting to databse: ".$this->db->connect_error.PHP_EOL;
			exit();
		}
	}

	public function __destruct()
	{
		if (isset($this->db))
		{
			$this->db->close();
		}
	}

	/*
	Checks the provided username and password against the database, logging them into the DB.
	Sets their access_level level associated with their logon.
	*/
	public function databaseLogin($username,$password)
	{
		$db_username = $this->db->real_escape_string($username);
		$db_password = $this->db->real_escape_string($password);

		$query = "select * from users where user_name = '$db_username';";
		$result= $this->db->query($query);
		while ($row = $result->fetch_assoc())
		{
			if ($row["password"] == $db_password)
			{
				$this->access_level = $row["access_level"];
				return true;
			}
		}
		return false;
	}

	/*
	This function will be called by all the modifying 
	*/
	private function access_level($req_access_level, $task_error_msg)
	{
		if($this->access_level >= $req_access_level)
		{
			return true;
		}
		else
		{
			echo "<h3>Do not have authorization to $task_error_msg.</h3>";
			return false;
		}
	}

		/*
	Takes .csv file output from ordering system and uploads into the database.
	Will insert into the orders and order_line_items tables (and into the inventory table if it does not exist there).
	Will mark items as on order in the inventory table.
	Order of .csv file is order_id, vendor_name, date_ordered, NSN, and quantity
	*/
	public function uploadOrder($filename)
	{
		//Make sure user has access_level to upload an order
		if(!$access_level=$this->access_level(self::USER, "upload an order"))
			return $access_level;
		echo "Can execute".PHP_EOL;

		/*
		Should really take the outputs and put them into a temporary file, then load that, so you can see what the output is before loading or in case of problems.
		*/
		if($file_input=fopen($filename, "r"))
		{
			$comp_order_id= NULL;
			$storeroom_id= NULL;
			while($data= fgetcsv($file_input))
			{
				$order_id= $data[0];

				//if its a new order_id, insert data into orders table
				if($comp_order_id != $order_id)
				{
					$comp_order_id= $order_id;
					$storeroom_name= $data[1];
					$query= "SELECT storeroom_id FROM storerooms WHERE storeroom_name = '$storeroom_name';";
					$result= $this->db->query($query);
					$row= $result->fetch_assoc();
					$storeroom_id= $row["storeroom_id"];
					$date_ordered= $data[2];

					$sql= "INSERT INTO orders (order_id, storeroom_id, date_ordered) VALUES ('$order_id', $storeroom_id, '$date_ordered');";
					$this->db->query($sql);
				}
				
				$NSN= $data[3];
				$quantity= $data[4];
				

				$sql= "INSERT INTO order_line_items (order_id, NSN, quantity) VALUES ('$order_id', $NSN, $quantity);";
				$this->db->query($sql);


				$sql= "UPDATE inventory SET on_order= 1 WHERE NSN=$NSN;";
				$this->db->query($sql);
			}

			fclose($file_input);
		}

	}

	/*
	error handling function for receiveOrder()
	*/
	private function receiveOrderError($order_id, $NSN)
	{
		echo "Could not receive item $NSN for order $order_id.".PHP_EOL;
		echo "Possible problems:\n1. Bad item number\n2. Bad order number\n3. Already been received".PHP_EOL;
		return false;
	}

	/*
	When personnel receive an order that had been placed.  
	For an order_id and NSN it will mark the current date as receiving that item in the order_line_item table,
	it will then increase the quantity for that NSN in the inventory table,
	then it will mark it as no longer on order in the inventory table.
	*/
	public function receiveOrder($order_id, $NSN)
	{
		//Make sure user has the access_level to receive an order.
		if(!$access_level=$this->access_level(self::USER, "receive an order"))
			return $access_level;
		
		//Mark the material as received with the current date in the order_line_items table
		$date= date("Y-m-d"); //current date
		$sql= "UPDATE order_line_items SET date_received='$date' WHERE order_id='$order_id' AND NSN=$NSN AND date_received IS NULL;";

		//Problem running query, or if after running no rows are affected (didn't update any rows),
		//meaning already received, or problem with $order_id, $NSN, or the combo of the two.
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			return $this->receiveOrderError($order_id, $NSN);
		}
		//Doesn't update any rows, meaning already received, or problem with $order_id, $NSN, or the combo of the two.
		
		//Get the storeroom that this item belongs in
		$query= "SELECT storeroom_id FROM orders WHERE order_id='$order_id';";
		$result= $this->db->query($query);
		$row= $result->fetch_assoc();

		$storeroom_id= $row['storeroom_id'];

		//Get the quantity ordered and quantity on hand in the inventory, then update the inventory with their sum
		$query= "SELECT order_line_items.quantity, inventory.quantity FROM order_line_items, inventory WHERE order_line_items.order_id='$order_id' AND order_line_items.NSN=$NSN AND order_line_items.NSN=inventory.NSN;";
		$result= $this->db->query($query);
		$row= $result->fetch_row();
		$new_inventory= $row[0]+$row[1];
		$sql= "UPDATE inventory SET quantity=$new_inventory, on_order=0 WHERE NSN=$NSN and storeroom_id=$storeroom_id;";

		//Problem running query, or after running, that no rows were updated
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			return $this->receiveOrderError($order_id, $NSN);
		}
		return true;
	}

	/*
	Gets a list of receivable order (order_id & NSN combinations)
	*/
	public function getReceivableOrders()
	{
		//Make sure user has the access_level to receive an order.
		if(!$access_level=$this->access_level(self::USER, "receive an order"))
			return $access_level;

		$query= "SELECT order_id, NSN FROM order_line_items	WHERE date_received IS NULL;";
		$result= $this->db->query($query);

		$html= "<h5>&ensp; Order Number &ensp;&nbsp; NSN</h5>";
		while($row=$result->fetch_assoc())
		{
			$order_id= $row['order_id'];
			$NSN= $row['NSN'];
			$combinedValue= $order_id.$NSN;
			$html .= "<div class='checkbox'><label><input type='checkbox' name='receivableOrder[]' value=$combinedValue>$order_id &emsp; $NSN</label></div>";
		}
		echo $html;
	}


	/*
	This function is used to mark that an item has been used.  
	It will reduce the quantity in the inventory by $quantity.
	It will then insert a new row into historical usage for that item and quantity at the time it is used (using the computer's clock).
	*/
	public function useItem($NSN, $quantity)
	{
		//Make sure user has the access_level to use an item.
		if(!$access_level=$this->access_level(self::USER, "use an item"))
			return $access_level;
		
		//Check that inventory.quantity is >= $quantity
		$query= "SELECT quantity FROM inventory WHERE NSN=$NSN;";
		$result= $this->db->query($query);
		$row= $result->fetch_assoc();
		$inventory_quantity=$row["quantity"];
		if($quantity>$inventory_quantity)
		{
			echo "$quantity is bigger than the quantity on hand: $inventory_quantity.".PHP_EOL;
			return false;
		}
		$inventory_quantity-=$quantity;
		$sql= "UPDATE inventory SET quantity=$inventory_quantity WHERE NSN=$NSN;";

		//Problem updating inventory's quantity
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			echo "problem with updating inventory's quantity".PHP_EOL;
			return false;
		}

		//Record the number of the item used in the historical usage table
		$datetime= date("Y-m-d H:i:s"); //current date
		$sql= "INSERT INTO historical_usage (date_used, NSN, quantity) VALUES ('$datetime', $NSN, $quantity);";
		//Problem updating inventory's quantity
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			echo "problem updating historical usage".PHP_EOL;
			return false;
		}
		return true;
	}

	/*
	Displays items and quantities so they can be selected to be used.
	*/
	public function getUsableItems()
	{
		//Make sure user has the access_level to use an item.
		if(!$access_level=$this->access_level(self::USER, "use an item"))
			return $access_level;

		$query= "SELECT NSN, storeroom_id FROM inventory;";
		$result= $this->db->query($query);

		$html= "<div class='form-group'>
					<h5>NSN</h5>
					<select class='form-control' name='usedItem'>
						<option disabled selected value> Select an NSN </option>";
		while($row=$result->fetch_assoc())
		{
			$NSN= $row['NSN'];
			$storeroom_id=$row['storeroom_id'];
			$combinedValue= $NSN.$storeroom_id;
			$html .= "<option value=$combinedValue>$NSN $emsp $storeroom_id</option>";
		}
		$html .= "</select><h5>Quantity</h5><input type='number' name='quantity'></div>";
		echo $html;
	}

	/*
	This function is used to update the par level (low water mark) of an item in the inventory.
	*/
	public function updateParLevel($NSN, $storeroom_id, $par_level)
	{
		//Make sure user has the access_level to update the par level.
		if(!$access_level=$this->access_level(self::USER, "update the par level"))
			return $access_level;

		//Checks the par level you want to change vs the current par level, if it is more than a 50% change then you need admin rights
		$query= "SELECT par_level from inventory WHERE NSN=$NSN AND storeroom_id=$storeroom_id;";
		$result= $this->db->query($query);
		$row= $result->fetch_assoc();
		$current_par_level= $row["par_level"];
		if((($par_level >= ($current_par_level*1.5)) ||  ($par_level <= ($current_par_level*0.5))) && 
			(!$access_level=$this->access_level(self::ADMIN, "update the par level by more than 50% of its current value")))
		{
			return $access_level;
		}

		$sql= "UPDATE inventory SET par_level=$par_level WHERE NSN=$NSN AND storeroom_id=$storeroom_id;";

		//Problem updating inventory's par_level
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			echo "problem with updating inventory's par_level".PHP_EOL;
			return false;
		}
		return true;
	}

	/*
	Get items from inventory table to be used to update par levels.
	*/
	public function getItemsParUpdate()
	{
		//Make sure user has the access_level to update the par level.
		if(!$access_level=$this->access_level(self::USER, "update the par level"))
			return $access_level;

		$query= "SELECT NSN, storeroom_id FROM inventory;";
		$result= $this->db->query($query);

		$html= "<div class='form-group'>
					<h5>NSN</h5>
					<select class='form-control' name='usedItem'>
						<option disabled selected value> Select an NSN </option>";
		while($row=$result->fetch_assoc())
		{
			$NSN= $row['NSN'];
			$storeroom_id= $row['storeroom_id'];
			$combinedValue= $NSN.$storeroom_id;
			$html .= "<option value=$combinedValue>$NSN &emsp $storeroom_id</option>";
		}
		$html .= "</select><h5>Quantity</h5><input type='number' name='par_level'></div>";
		echo $html;
	}

	/*
	This function will remove an item from inventory if it is no longer carried
	*/
	public function deleteItemFromInventory($NSN)
	{
		//Make sure user has the access_level to remove an item from inventory.
		if(!$access_level=$this->access_level(self::USER, "remove item from inventory"))
			return $access_level;

		$query= "SELECT quantity FROM inventory WHERE NSN=$NSN;";
		$result= $this->db->query($query);
		$row=$result->fetch_assoc();
		if($row["quantity"]>0)
		{
			echo "Can't remove $NSN from inventory because it is still in stock.".PHP_EOL;
			return false;
		}

		$sql= "DELETE FROM inventory WHERE NSN=$NSN;";
		$this->db->query($sql);

	}


	/*
	Runs a report of items on inventory
	*/
	public function reportInventory()
	{
		//Make sure user has access_level to run inventory report. Everyone as of now.
		if(!$access_level=$this->access_level(self::GUEST, "run an Inventory Report"))
			return $access_level;
		
		$query= "SELECT storerooms.storeroom_name, inventory.NSN, items.item_name, items.item_desc, inventory.quantity, inventory.par_level
			FROM inventory, items, storerooms WHERE inventory.NSN=items.NSN AND storerooms.storeroom_id=inventory.storeroom_id;";
		$result = $this->db->query($query);
		$html= "<div class='container'><div class='row'><div class='col-md'12'>
				<table class='table table-bordered table-striped table-condensed table-responsive'>
					<thead>
						<tr>
							<th>Storeroom</th>
							<th>National Stock Number</th>
							<th>Item Name</th>
							<th>Item Description</th>
							<th>Quantity</th>
							<th>Par Level</th>
						</tr>
					</thead>
					<tbody>";
		while ($row = $result->fetch_row())
		{
			$html .= "<tr>";
			for($i=0; $i<sizeof($row); $i++)
			{
				if($i==6)
				{
					if($row[$i]==1)
					{
						$row[$i]="Yes";
					}
					else
					{
						$row[$i]="No";
					}
				}
				$html .= "<td> $row[$i] </td>";
			}
			$html .= "</tr>";
		}
		$html .= "	</tbody>
				</table>
				</div></div></div>";
		echo $html;
	}

	/*
	Runs a report of items on order
	*/
	public function reportItemsOrdered()
	{
		//Reports of items on order available to everyone as of now
		if(!$access_level=$this->access_level(self::GUEST, "run an On Order Report"))
			return $access_level;
		
		
		$query="SELECT orders.order_id, storerooms.storeroom_name, order_line_items.NSN, items.item_name, order_line_items.quantity, orders.date_ordered, order_line_items.date_received
		FROM orders, order_line_items, items, storerooms
		WHERE orders.order_id=order_line_items.order_id 
		AND order_line_items.NSN=items.NSN
		AND storerooms.storeroom_id=orders.storeroom_id;";
		$result = $this->db->query($query);
		$html= "<div class='container'><div class='row'><div class='col-md-12'>
				<table class='table table-bordered table-striped table-condensed table-responsive'>
					<thead>
						<tr>
							<th>Order Number</th>
							<th>Storeroom</th>
							<th>NSN</th>
							<th>Item Name</th>
							<th>Quantity</th>
							<th>Date Ordered</th>
							<th>Date Received</th>
						</tr>
					</thead>
					<tbody>";
		while ($row = $result->fetch_row())
		{
			$html .= "<tr>";
			for($i=0; $i<sizeof($row); $i++)
			{
				$html .= "<td> $row[$i] </td>";
			}
			$html .= "</tr>";
		}
		$html .= "	</tbody>
				</table>
				</div></div></div>";
		echo $html;
	}

	/*
	Runs a report of items at or below their par level (low water mark)
	*/
	public function reportItemsBelowParLevel()
	{
		//Reports of items at or below par level available to everyone as of now
		if(!$access_level=$this->access_level(self::GUEST, "run a Par Level Report"))
			return $access_level;
		
		
		$query="SELECT storerooms.storeroom_name, inventory.NSN, items.item_name, inventory.quantity, inventory.par_level 
			FROM inventory, items, storerooms 
			WHERE inventory.NSN=items.NSN AND 
			inventory.storeroom_id=storerooms.storeroom_id AND
			inventory.quantity<=inventory.par_level;";
		$result = $this->db->query($query);
		$html= "<div class='container'><div class='row'><div class='col-md-12'>
				<table class='table table-bordered table-striped table-condensed table-responsive' style='margin: auto; width:50%;'>
					<thead>
						<tr>
							<th>Storeroom</th>
							<th>NSN</th>
							<th>Item Name</th>
							<th>Quantity</th>
							<th>Par Level</th>
						</tr>
					</thead>
					<tbody>";
		while ($row = $result->fetch_row())
		{
			
			$html .= "<tr>";
			for($i=0; $i<sizeof($row); $i++)
			{
				$html .= "<td> $row[$i] </td>";
			}
			$html .= "</tr>";
		}
		$html .= "	</tbody>
				</table>
				</div></div></div>";
		echo $html;
	}

	/*
	Gets the database's recommended par levels.  
	The par level should be enough that when supplies are reordered, they will have more than enough time to receive the items before running out.
	*/
	public function reportRecommendedParLevels()
	{
		//Reports of recommended par levels for items.   
		if(!$access_level=$this->access_level(self::GUEST, "run a Recommended Par Level Report"))
			return $access_level;

		$query="SELECT storerooms.storeroom_name, historical_usage.NSN, items.item_name, MIN(historical_usage.date_used), MAX(historical_usage.date_used), SUM(historical_usage.quantity), historical_usage.storeroom_id
			FROM historical_usage, items, storerooms
			WHERE items.NSN=historical_usage.NSN AND
			storerooms.storeroom_id=historical_usage.storeroom_id
			Group By historical_usage.NSN;";
		$result= $this->db->query($query);
		$html= "<div class='container'><div class='row'><div class='col-md-12'>
				<table class='table table-bordered table-striped table-condensed table-responsive' style='margin: auto; width:50%;'>
					<thead>
						<tr>
							<th>Storeroom</th>
							<th>NSN</th>
							<th>Item Name</th>
							<th>Recommended Par Level</th>
						</tr>
					</thead>
					<tbody>";
		while($row= $result->fetch_row())
		{
			$storeroom_name=$row[0];
			$NSN=$row[1];
			$item_name=$row[2];

			$old_date= new DateTime($row[3]);
			$new_date= new DateTime($row[4]);
			$diff= $old_date->diff($new_date);
			$ave_use=$row[4]/($diff->days); //average number of item used per day (BURN RATE)
			
			$storeroom_id=$row[5];
			//Get the average time it takes for an order to be delivered for this item
			$query2= "SELECT AVG(datediff(order_line_items.date_received, orders.date_ordered)) 
				FROM orders, order_line_items 
				WHERE order_line_items.order_id=orders.order_id
				AND order_line_items.NSN=$NSN
				AND orders.storeroom_id=$storeroom_id;";
			$result2= $this->db->query($query2);
			$row2=$result2->fetch_row();

			$par_level=2*$ave_use*$row2[0];//double the order time * burn rate
			$html .= "<tr>
						<td> $storeroom_name </td> 
						<td> $NSN </td>
						<td> $item_name </td>
						<td> $par_level </td>
					</tr>";
		}
		$html .= "	</tbody>
				</table>
				</div></div></div>";
		echo $html;
	}

	/*
	Get the historical usage of items in the database.
	*/
	public function reportHistoricalUsage()
	{
		//Reports of historical usage of items.   
		if(!$access_level=$this->access_level(self::GUEST, "run a Historical Usage of Items Report"))
			return $access_level;

		$query="SELECT historical_usage.date_used, storerooms.storeroom_name, historical_usage.NSN, items.item_name, historical_usage.quantity
			FROM historical_usage, items, storerooms
			WHERE items.NSN=historical_usage.NSN 
			AND historical_usage.storeroom_id=storerooms.storeroom_id
			ORDER BY historical_usage.date_used DESC;";
		$result= $this->db->query($query);
		$html= "<div class='container'><div class='row'><div class='col-md-12'>
				<table class='table table-bordered table-striped table-condensed table-responsive' style='margin: auto; width:50%;'>
					<thead>
						<tr>
							<th>Date Used</th>
							<th>Storeroom</th>
							<th>NSN</th>
							<th>Item Name</th>
							<th>Quantity Used</th>
						</tr>
					</thead>
					<tbody>";
		while ($row = $result->fetch_row())
		{
			
			$html .= "<tr>";
			for($i=0; $i<sizeof($row); $i++)
			{
				$html .= "<td> $row[$i] </td>";
			}
			$html .= "</tr>";
		}
		$html .= "	</tbody>
				</table>
				</div></div></div>";
		echo $html;
	}

	/*
	Add an item to the database.
	Supply technicians (USER) should be able to add items smartly (be trained),
	rather than only being an ADMIN role.
	*/
	public function ADMIN_addItem($NSN,$item_name,$item_desc)
	{
		//Check if user has access_level to add an item
		if(!$access_level=$this->access_level(self::ADMIN, "add an item"))
			return $access_level;
		
		$query="INSERT INTO items (NSN, item_name, item_desc) VALUES ('$NSN', '$item_name', '$item_desc');";
		if(!$this->db->query($query))
		{
			echo "failed to add $NSN".PHP_EOL;
		}

		$query="INSERT INTO inventory (NSN, quantity) VALUES ('$NSN', 0);";
		if(!$this->db->query($query))
		{
			echo "failed to add $NSN to inventory".PHP_EOL;
		}
	}

	/*
	Delete an item from the items table (only available to admin)
	*/
	public function ADMIN_deleteItem($NSN)
	{
		//Check if admin rights to delete an item from items table
		if(!$access_level=$this->access_level(self::ADMIN, "delete an Item"))
			return $access_level;

		$sql= "DELETE FROM inventory WHERE NSN=$NSN;";
		if(!$this->db->query($sql))
		{
			echo "Error deleting $NSN from the DB.".PHP_EOL;
			return false;
		}

		$sql= "DELETE FROM items WHERE NSN=$NSN;";
		if(!$this->db->query($sql))
		{
			echo "Error deleting $NSN from the DB.".PHP_EOL;
			return false;
		}
		return true;
	}

	/*
	Update the inventory quantity.  
	Used for when a mistake was made in using or receiving an item, or for gains/loss by physical inventory.
	*/
	public function ADMIN_updateInventoryQuantity($NSN, $storeroom_id, $quantity)
	{
		//Check if admin rights to update the quantity in the inventory
		if(!$access_level=$this->access_level(self::ADMIN, "update inventory quantity"))
			return $access_level;

		$sql= "UPDATE inventory SET quantity=$quantity WHERE NSN=$NSN AND storeroom_id=$storeroom_id;";

		//Problem updating inventory's quantity
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			echo "problem with updating inventory's quantity".PHP_EOL;
			return false;
		}
	}

	/*
	Get the item id for items in inventory, to be used with ADMIN update inventory quantity and update inventory information
	*/
	public function ADMIN_getUpdatableInventoryQuantity()
	{
		//Check if admin rights to update information in the inventory
		if(!$access_level=$this->access_level(self::ADMIN, "update inventory information"))
			return $access_level;

		$query= "SELECT NSN, storeroom_id, quantity FROM inventory;";
		$result= $this->db->query($query);

		$html= "<div class='form-group'><h5>NSN &emsp;&emsp;&emsp;&emsp; Current Quantity</h5>
			<select class='form-control' name= 'NSN'>
				<option disabled selected value> Select an NSN </option>";
		while($row=$result->fetch_assoc())
		{
			$NSN= $row['NSN'];
			$storeroom_id= $row['storeroom_id'];
			$combinedValue= $NSN.$storeroom_id;
			$quantity= $row['quantity'];
			$html .= "<option value= '$combinedValue'>$NSN &emsp; $storeroom_id &emsp; $quantity</option>";
		}
		$html .= "</select><h5>New Quantity</h5><input type='number' name='quantity'></div>";
		echo $html;
	}

	/*
	Update the inventory quantity.  
	Used for when a mistake was made in using or receiving an item, or for gains/loss by physical inventory.
	*/
	public function ADMIN_updateInventoryInfo($NSN, $storeroom_id, $quantity, $par_level,)
	{
		//Check if admin rights to update the quantity in the inventory
		if(!$access_level=$this->access_level(self::ADMIN, "update inventory information"))
			return $access_level;

		$sql= "UPDATE inventory SET quantity=$quantity, par_level=$par_level WHERE NSN=$NSN AND storeroom_id=$storeroom_id;";

		//Problem updating inventory's quantity
		if(!$this->db->query($sql) || $this->db->affected_rows==0)
		{
			echo "problem with updating inventory's info".PHP_EOL;
			return false;
		}
	}

	/*
	Get the item id for items in inventory, to be used with ADMIN update inventory quantity and update inventory information
	*/
	public function ADMIN_getUpdatableInventoryInfo()
	{
		//Check if admin rights to update information in the inventory
		if(!$access_level=$this->access_level(self::ADMIN, "update inventory information"))
			return $access_level;

		$query= "SELECT NSN, storeroom_id FROM inventory;";
		$result= $this->db->query($query);

		$html= "<div class='form-group'><h5>NSN</h5>
			<select class='form-control' name= 'NSN'>
				<option disabled selected value> Select an NSN </option>";
		while($row=$result->fetch_assoc())
		{
			$NSN= $row['NSN'];
			$storeroom_id= $row['storeroom_id'];
			$combinedValue= $NSN.$storeroom_id;
			$html .= "<option value= '$combinedValue'>$NSN &emsp; $storeroom_id</option>";
		}
		$html .= "</select></div>";
		echo $html;
	}

	/*
	Delete an order from the orders database (only available to admin)
	*/
	public function ADMIN_deleteOrder($order_id)
	{
		//Check if admin rights to delete an order
		if(!$access_level=$this->access_level(self::ADMIN, "delete an Order"))
			return $access_level;

		//delete from order_line_items first, because its a child table of orders
		$sql= "DELETE FROM order_line_items WHERE order_id='$order_id';";
		if(!$this->db->query($sql))
		{
			echo "Error deleting $order_id from the order_line_items table.".PHP_EOL;
			return false;
		}

		//delete from orders table
		$sql= "DELETE FROM orders WHERE order_id='$order_id';";
		if(!$this->db->query($sql))
		{
			echo "Error deleting $order_id from the orders table.".PHP_EOL;
			return false;
		}	
		return true;
	}

	/*
	Get the order ids for orders not received yet, able to be deleted
	*/
	public function ADMIN_getDeletableOrders()
	{
		//Check if admin rights to update information in the inventory
		if(!$access_level=$this->access_level(self::ADMIN, "delete an Order"))
			return $access_level;

		$query= "SELECT order_id FROM order_line_items WHERE date_received IS NULL GROUP BY order_id;";
		$result= $this->db->query($query);

		$html= "<div class='form-group'><h5>Order ID</h5>
			<select class='form-control' name= 'order_id'>
				<option disabled selected value> Select an Order ID </option>";
		while($row=$result->fetch_assoc())
		{
			$order_id= $row['order_id'];
			$html .= "<option value= '$order_id'>$order_id</option>";
		}
		$html .= "</select></div>";
		echo $html;
	}

}

?>