CREATE DATABASE `iwg_asset_management` /*!40100 COLLATE 'utf8_general_ci' */;

use iwg_asset_management;

CREATE TABLE `tb_gweui_resource_definition` (
	`name` VARCHAR(32) NOT NULL,
	`value` VARCHAR(32) NOT NULL,
	`description` VARCHAR(64) NOT NULL,
	PRIMARY KEY (`name`, `value`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;

CREATE TABLE `tb_gweui_secret_key` (
	`id` BIGINT(20) UNSIGNED NOT NULL AUTO_INCREMENT,
	`gweui` VARCHAR(32) NOT NULL,
	`secret_key` VARCHAR(64) NOT NULL,
	`creator` INT(10) UNSIGNED NOT NULL,
	`create_time` DATETIME NOT NULL,
	`status` SMALLINT(6) NOT NULL COMMENT '-1:弃用;0:未分配;1:已分配',
	`operate_serial_list` VARCHAR(256) NULL DEFAULT NULL,
	PRIMARY KEY (`id`),
	UNIQUE INDEX `gweui` (`gweui`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
AUTO_INCREMENT=1
;

CREATE TABLE `tb_gweui_usage` (
	`oui` VARCHAR(8) NOT NULL,
	`rate_range` VARCHAR(8) NOT NULL,
	`power_type` VARCHAR(8) NOT NULL,
	`data_back` VARCHAR(8) NOT NULL,
	`factory` VARCHAR(8) NOT NULL,
	`serial` VARCHAR(8) NOT NULL,
	`space` INT(10) UNSIGNED NOT NULL,
	PRIMARY KEY (`oui`, `rate_range`, `power_type`, `data_back`, `factory`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
;

CREATE TABLE `tb_logs` (
	`id` INT(10) UNSIGNED NOT NULL AUTO_INCREMENT,
	`serial` CHAR(21) NOT NULL,
	`operator` INT(11) NOT NULL,
	`operate_time` DATETIME NOT NULL,
	`operate_type` VARCHAR(16) NOT NULL,
	`message` VARCHAR(256) NULL DEFAULT NULL,
	PRIMARY KEY (`id`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
AUTO_INCREMENT=1
;

CREATE TABLE `tb_user` (
	`id` SMALLINT(5) UNSIGNED NOT NULL AUTO_INCREMENT,
	`name` VARCHAR(32) NOT NULL,
	`description` VARCHAR(50) NOT NULL,
	`pwd_md5` VARCHAR(32) NOT NULL,
	`permission_list` VARCHAR(50) NOT NULL,
	`status` TINYINT(4) NULL DEFAULT NULL,
	PRIMARY KEY (`id`)
)
COLLATE='utf8_general_ci'
ENGINE=InnoDB
AUTO_INCREMENT=1
;

