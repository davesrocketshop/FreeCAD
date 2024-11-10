USE material;

DROP TABLE IF EXISTS library;
CREATE TABLE library (
	library_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	library_name VARCHAR(512) NOT NULL UNIQUE,
	library_icon BLOB,
	library_read_only TINYINT(1) NOT NULL DEFAULT 0
);

DROP TABLE IF EXISTS folder;
CREATE TABLE folder (
	folder_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	folder_name VARCHAR(512) NOT NULL UNIQUE,
	library_id INTEGER NOT NULL,
	parent_id INTEGER,
	FOREIGN KEY (library_id)
        REFERENCES library(library_id),
	FOREIGN KEY (parent_id)
        REFERENCES folder(folder_id)
);

DROP TABLE IF EXISTS model;
CREATE TABLE model (
    model_id CHAR(36) NOT NULL PRIMARY KEY,
	library_id INTEGER NOT NULL,
	folder_id INTEGER,
	model_type ENUM('Model', 'AppearanceModel') NOT NULL,
	model_name VARCHAR(1024) NOT NULL,
	model_url VARCHAR(1024),
	model_description TEXT,
	model_doi VARCHAR(1024),
	FOREIGN KEY (library_id)
        REFERENCES library(library_id),
	FOREIGN KEY (folder_id)
        REFERENCES folder(folder_id)
);

DROP TABLE IF EXISTS model_inheritance;
CREATE TABLE model_inheritance (
	model_inheritance_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	model_id CHAR(36) NOT NULL,
	inherits_id CHAR(36) NOT NULL,
	FOREIGN KEY (model_id)
        REFERENCES model(model_id),
	FOREIGN KEY (inherits_id)
        REFERENCES model(model_id)
);

DROP TABLE IF EXISTS model_property;
CREATE TABLE model_property (
    model_property_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	model_id CHAR(36) NOT NULL,
	model_property_name VARCHAR(1024) NOT NULL,
	model_property_display_name VARCHAR(1024) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(1024) NOT NULL,
	model_property_description TEXT,
	model_property_inheritance_id CHAR(36),
	FOREIGN KEY (model_id)
        REFERENCES model(model_id),
	FOREIGN KEY (model_property_inheritance_id)
        REFERENCES model(model_id)
);

DROP TABLE IF EXISTS model_property_column;
CREATE TABLE model_property_column (
    model_property_column_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
    model_property_id INTEGER NOT NULL,
	model_property_name VARCHAR(1024) NOT NULL,
	model_property_display_name VARCHAR(1024) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(1024) NOT NULL,
	model_property_description TEXT,
	FOREIGN KEY (model_property_id)
        REFERENCES model_property(model_property_id)
);
