USE material;

-- Foreign keys require tables to be dropped in a specific sequence
SET FOREIGN_KEY_CHECKS=0;

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
	folder_name VARCHAR(512) NOT NULL,
	library_id INTEGER NOT NULL,
	parent_id INTEGER,
	FOREIGN KEY (library_id)
        REFERENCES library(library_id)
		ON DELETE CASCADE,
	FOREIGN KEY (parent_id)
        REFERENCES folder(folder_id)
);

DROP TABLE IF EXISTS model;
CREATE TABLE model (
    model_id CHAR(36) NOT NULL PRIMARY KEY,
	library_id INTEGER NOT NULL,
	folder_id INTEGER,
	model_type ENUM('Model', 'AppearanceModel') NOT NULL,
	model_name VARCHAR(255) NOT NULL,
	model_url VARCHAR(255),
	model_description TEXT,
	model_doi VARCHAR(255),
	FOREIGN KEY (library_id)
        REFERENCES library(library_id)
		ON DELETE CASCADE,
	FOREIGN KEY (folder_id)
        REFERENCES folder(folder_id)
		ON DELETE CASCADE
);

DROP TABLE IF EXISTS model_inheritance;
CREATE TABLE model_inheritance (
	model_inheritance_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	model_id CHAR(36) NOT NULL,
	inherits_id CHAR(36) NOT NULL,
	FOREIGN KEY (model_id)
        REFERENCES model(model_id)
		ON DELETE CASCADE,
	FOREIGN KEY (inherits_id)
        REFERENCES model(model_id)
		ON DELETE RESTRICT
);

DROP TABLE IF EXISTS model_property;
CREATE TABLE model_property (
    model_property_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	model_id CHAR(36) NOT NULL,
	model_property_name VARCHAR(255) NOT NULL,
	model_property_display_name VARCHAR(255) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(255) NOT NULL,
	model_property_description TEXT,
	FOREIGN KEY (model_id)
        REFERENCES model(model_id)
		ON DELETE CASCADE
);

DROP TABLE IF EXISTS model_property_column;
CREATE TABLE model_property_column (
    model_property_column_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
    model_property_id INTEGER NOT NULL,
	model_property_name VARCHAR(255) NOT NULL,
	model_property_display_name VARCHAR(255) NOT NULL,
	model_property_type VARCHAR(255) NOT NULL,
	model_property_units VARCHAR(255) NOT NULL,
	model_property_url VARCHAR(255) NOT NULL,
	model_property_description TEXT,
	FOREIGN KEY (model_property_id)
        REFERENCES model_property(model_property_id)
		ON DELETE CASCADE
);

DROP TABLE IF EXISTS material;
CREATE TABLE material (
    material_id CHAR(36) NOT NULL PRIMARY KEY,
	library_id INTEGER NOT NULL,
	folder_id INTEGER,
	material_name VARCHAR(255) NOT NULL,
	material_author VARCHAR(255),
	material_license VARCHAR(255),
	material_parent_uuid CHAR(36),
	material_description TEXT,
	material_url VARCHAR(255),
	material_reference VARCHAR(255),
	FOREIGN KEY (library_id)
        REFERENCES library(library_id)
		ON DELETE CASCADE,
	FOREIGN KEY (folder_id)
        REFERENCES folder(folder_id)
		ON DELETE CASCADE,
	FOREIGN KEY (material_parent_uuid)
        REFERENCES material(material_id)
		ON DELETE RESTRICT
);

DROP TABLE IF EXISTS material_tag;
CREATE TABLE material_tag (
    material_tag_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	material_tag_name VARCHAR(255) NOT NULL UNIQUE KEY
);

DROP TABLE IF EXISTS material_tag_mapping;
CREATE TABLE material_tag_mapping (
    material_id CHAR(36) NOT NULL,
	material_tag_id INTEGER NOT NULL,
	FOREIGN KEY (material_id)
        REFERENCES material(material_id)
		ON DELETE CASCADE,
	FOREIGN KEY (material_tag_id)
        REFERENCES material_tag(material_tag_id)
		ON DELETE CASCADE
);
CREATE INDEX material_tag_mapping_material_id ON material_tag_mapping (material_id);
CREATE INDEX material_tag_mapping_material_tag_id ON material_tag_mapping (material_tag_id);

DROP TABLE IF EXISTS material_models;
CREATE TABLE material_models (
    material_id CHAR(36) NOT NULL,
    model_id CHAR(36) NOT NULL,
	FOREIGN KEY (material_id)
        REFERENCES material(material_id)
		ON DELETE CASCADE,
	FOREIGN KEY (model_id)
        REFERENCES model(model_id)
		ON DELETE CASCADE
);
CREATE INDEX material_models_material_id ON material_models (material_id);

DROP TABLE IF EXISTS material_property_value;
CREATE TABLE material_property_value (
    material_property_value_id INTEGER AUTO_INCREMENT NOT NULL PRIMARY KEY,
	material_id CHAR(36) NOT NULL,
	material_property_name VARCHAR(255) NOT NULL,
	material_property_value TEXT NOT NULL,
	FOREIGN KEY (material_id)
        REFERENCES material(material_id)
		ON DELETE CASCADE
);

-- Restore foreign key checks
SET FOREIGN_KEY_CHECKS=1;
