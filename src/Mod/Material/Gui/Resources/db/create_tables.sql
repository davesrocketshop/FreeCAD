USE material;

DROP TABLE IF EXISTS library;
-- CREATE TABLE library (
-- 	library_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
-- 	library_name VARCHAR(1024) NOT NULL UNIQUE, - doesn't like the UNIQUE, key is too long
-- 	library_icon BLOB,
-- 	library_read_only TINYINT(1) NOT NULL DEFAULT 0
-- );
CREATE TABLE library (
	library_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	library_name VARCHAR(1024) NOT NULL,
	library_icon BLOB,
	library_read_only TINYINT(1) NOT NULL DEFAULT 0
);

DROP TABLE IF EXISTS model;
CREATE TABLE model (
    model_id CHAR(36) NOT NULL PRIMARY KEY,
	library_id INTEGER NOT NULL, -- foreign key
	model_name VARCHAR(1024) NOT NULL,
	model_url VARCHAR(1024),
	model_description TEXT,
	model_doi VARCHAR(1024)
);

DROP TABLE IF EXISTS model_property;
CREATE TABLE model_property (
    model_property_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	model_id CHAR(36) NOT NULL, -- foreign key
	model_property_name VARCHAR(1024) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(1024) NOT NULL,
	model_property_description TEXT
);

DROP TABLE IF EXISTS model_property_column;
CREATE TABLE model_property_column (
    model_property_column_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
    model_property_id INTEGER NOT NULL,
	model_property_name VARCHAR(1024) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(1024) NOT NULL,
	model_property_description TEXT
);
