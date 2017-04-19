/*
Loads data into each of the tables of the database.
*/
LOAD DATA LOCAL INFILE 'csvFiles/items.csv'
INTO TABLE items 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(NSN, item_name, item_desc);

LOAD DATA LOCAL INFILE 'csvFiles/storerooms.csv'
INTO TABLE storerooms 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(storeroom_name);

LOAD DATA LOCAL INFILE 'csvFiles/orders.csv'
INTO TABLE orders 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(order_id, storeroom_id, date_ordered);

LOAD DATA LOCAL INFILE 'csvFiles/order_line_items.csv'
INTO TABLE order_line_items 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(order_id, NSN, quantity, date_received);

LOAD DATA LOCAL INFILE 'csvFiles/inventory.csv'
INTO TABLE inventory 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(NSN, storeroom_id, quantity, par_level);

LOAD DATA LOCAL INFILE 'csvFiles/users.csv'
INTO TABLE users 
	FIELDS TERMINATED BY ','
		OPTIONALLY ENCLOSED BY '"'
	LINES TERMINATED BY '\n'
(user_name, password, access_level);