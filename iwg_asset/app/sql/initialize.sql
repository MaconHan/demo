use iwg_asset_management;

INSERT INTO `tb_user` (`name`, `description`, `pwd_md5`, `permission_list`, `status`) VALUES ('admin', 'admin', '53b68a6ba422cbec859edc26446ce9e0', 'all', 1);

INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('data_back', '0', '有线FE');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('data_back', '1', '无线3G/4G');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('data_back', '2', '无线3G/4G+无线WIFI');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('data_back', '3', '无线WIFI');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('data_back', '4', '无线专网LTE');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('factory', '0', '中兴克拉');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('gweui_status', '-1', '弃用');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('gweui_status', '0', '未分配');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('gweui_status', '1', '已分配');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('oui', '6073BC', 'IWG 200');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('power_type', '0', '12V直流供电');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('power_type', '1', '太阳能供电');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '00', '全双工:上行480-490MHz/下行500-510MHz');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '01', '全双工:上行470-480MHz/下行490-500MHz');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '02', '半双工:470-490MHz');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '03', '半双工:490-510MHz');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '04', '半双工:480-500MHz');
INSERT INTO `tb_gweui_resource_definition` (`name`, `value`, `description`) VALUES ('rate_range', '05', '半双工:470-478MHz');

