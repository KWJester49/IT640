/*
Creates the items table.
This will hold information about the items the inventory has or has had.
*/
DROP TABLE IF EXISTS items;

CREATE TABLE items(
	NSN bigint(13) NOT NULL, 
	/* 
	The primary key for the items table.
	Identification number of the item, unique for each item regardless of manufacturer.  
	Navy uses a 13 digit number called an NSN (National Stock Number), so I reused that here. 
	NSN is not determined locally, but assigned to items by NATO.
	Items will have the same NSN regardless of who manufactured it. 
	*/
	item_name varchar(255) NOT NULL, 
	/*
	Name of the item.
	*/
	item_desc varchar(255) DEFAULT NULL, 
	/*
	Description of the item.
	*/
	PRIMARY KEY (NSN)
);

/*
Creates the storerooms table, for all the different Supply Department storerooms on the ship.
*/
DROP TABLE IF EXISTS storerooms;

CREATE TABLE storerooms (
	storeroom_id tinyint NOT NULL AUTO_INCREMENT,
	/*
	Primary key for the storerooms table.
	*/
	storeroom_name varchar(255) NOT NULL,
	/*
	Name of the storeroom
	*/
	PRIMARY KEY (storeroom_id)
);

/*
Creates inventory table.  
Connects to items table using NSN as foreign key.
Connects to storerooms table using storeroom_id
Used to track what is currently on stock in the store rooms' inventories.
Items can potentially be in multiple storerooms, composite key based on NSN and storeroom_id.
*/
DROP TABLE IF EXISTS inventory;

CREATE TABLE inventory (
	NSN bigint(13) NOT NULL, 
	/* 
	Foreign key from items table.  
	*/
	storeroom_id tinyint NOT NULL,
	/*
	Foregin key from storerooms table.
	Which storeroom this is, combined with NSN to make unique key
	*/
	quantity int(5) NOT NULL, 
	/*
	Quantity on hand of the item.
	*/
	par_level int(4) DEFAULT NULL, 
	/*
	The reorder point for the item.  When quantity is below this, reorder.
	*/
	PRIMARY KEY (NSN, storeroom_id), 
	/*
	Composite key of the item and where it is stored.
	*/
	FOREIGN KEY (NSN) REFERENCES items(NSN),
	FOREIGN KEY (storeroom_id) REFERENCES storerooms(storeroom_id)
);

/*
Creates the historical_usage table.
Stores when all items stored in the inventory table are used.
*/
DROP TABLE IF EXISTS historical_usage;

CREATE TABLE historical_usage (
	date_used datetime NOT NULL,
	/*
	When an item from inventory is used.
	*/
	NSN bigint(13) NOT NULL, 
	/*
	The item from the inventory used.
	*/
	storeroom_id tinyint NOT NULL,
	/*
	Foregin key from storerooms table.
	Which storeroom this is, combined with NSN to make unique key
	*/
	quantity int(5) NOT NULL, 
	/*
	The number of the item used.
	*/
	PRIMARY KEY (date_used, NSN, storeroom_id), 
	/*
	Composite key of the item used and when it was used.
	*/
	FOREIGN KEY (NSN) REFERENCES items(NSN),
	FOREIGN KEY (storeroom_id) REFERENCES storerooms(storeroom_id)
);

/*
Creates the orders table.
Connects to the storerooms table using storeroom_id as a foreign key.
Keeps track of all the orders that have been placed, whether they have been received or not.
New orders will be made through another system, which will generate a .csv file to update the database.
Each storeroom will make orders separately, so it will be marked in the order who will get it when it arrives.
*/
DROP TABLE IF EXISTS orders;

CREATE TABLE orders (
	order_id varchar(14) NOT NULL,
	/*
	Primary key of the orders table.
	Order number (order_id) will be provided by the .csv file.
	*/
	storeroom_id tinyint NOT NULL,
	/*
	Foregin key from storerooms table.
	Which storeroom this is, combined with NSN to make unique key
	*/
	date_ordered date NOT NULL,
	/*
	The date that the order is placed on.
	*/
	PRIMARY KEY (order_id),
	FOREIGN KEY (storeroom_id) REFERENCES storerooms(storeroom_id)
);

/*
Creates order_line_items table.
Connects to the orders table using order_id as a foreign key.
Connects to the items table using NSN as a foreign key.
Stores information about each item that is in an order.
Table is necessary because it is quite common to order multiple things at once from the same vendor.
A line item is uniquely defined based off of the order_id and item_id.
*/
DROP TABLE IF EXISTS order_line_items;

CREATE TABLE order_line_items (
	order_id varchar(14) NOT NULL,
	/*
	Foreign key from the orders table.
	The overall order of which this line item belongs to.
	*/
	NSN bigint(13) NOT NULL, 
	/*
	Foreign key from the items table.
	The item being ordered on this line item.
	*/
	quantity int(4) NOT NULL, 
	/*
	Quantity being ordered.
	*/
	date_received date DEFAULT NULL,
	/*
	The date when the item is received.
	This implementation is assuming that specific line items of an order may not be delivered at the same time, 
	but that all of a single line item WILL be delivered at the same time.
	In real life this might not actually occur, partial delivery of a single line item may occur, but for the purposes of this project and simplicity I opted for this.
	*/
	PRIMARY KEY (order_id, NSN),
	/*
	Creates a composite key of order_id and item_id to uniquely identify each line item of an order.
	*/
	FOREIGN KEY (order_id) REFERENCES orders(order_id),
	FOREIGN KEY (NSN) REFERENCES items(NSN)
);

/*
Creates the users table.
Used to determine permissions when using this database.
There will be a blank user_name and password for guests.
*/
DROP TABLE IF EXISTS users;

CREATE TABLE users (
	user_id int(4) NOT NULL AUTO_INCREMENT, 
	/*
	Primary key of the users table.
	Internal identification number for user, just auto increment for each new user.
	*/
	user_name varchar(10) NOT NULL UNIQUE,
	/*
	The users username, which I am requiring to be unique in the database.
	The format will be the first initial followed by up to the first 9 letters of the last name, all caps.
	Example: My user_name would be BNICHOLS.
	*/
	password varchar(20) DEFAULT NULL,
	/*
	The password for the username.
	*/
	access_level int(1) DEFAULT 0,
	/*
	I could create two more tables, one of admin rights, specifying the level (as the primary key) with another column saying what that particular value provides.
	Then there would be another table for user_permissions which would have the user_id and admin_rights.
	This would allow the ability to do fine granularity in what each user will have access to.
	As this is a very simple project, I am only doing tiered permissions, where the higher the number, the more permissions they have.
	Admin user will have 5, guest will have 0, other users right now have 3.  This gives expansion room if I decide later to add more levels in between at a later time.
	*/
	PRIMARY KEY (user_id)
);