一、说明：iwg_asset IWG资产管理系统，管理gweui号码和秘钥

二、主要功能：
1、生成gweui，并支持原文和加密导出文件
2、根据gweui，并支持批量加密导出文件
3、查询当前用户操作日志，并支持再导出文件
4、用户登录和登出

三、编译、安装和启动步骤：
1、编译版本 ./iwg_asset.sh -build 
1、到version目录下下载iwg_asset.tar.gz版本包
2、将该版本包解压缩到待安装的主机上
3、确保待安装的主机已经安装有docker并启动
注意：docker服务启动配置不要设置--selinux-enabled，请在/etc/sysconfig/docker配置文件的OPTIONS中删除该配置项。修改参数重启docker，命令如：service docker restart

4、安装执行: ./iwg_asset.sh -install
    数据库默认端口是：3306。如果docker所在的主机上该端口冲突，可修改iwg_asset.sh文件中db_port参数的值
    web服务器默认端口是：5000。如果docker所在的主机上该端口冲突，可修改iwg_asset.sh文件中web_port参数的值
    修改服务端口，请在制作版本前修改iwg_asset.sh中db和web配置
    
5、启动执行：./iwg_asset.sh -start
    请使用浏览器访问web服务，例如：https://host:5000。因采用非合法证书，请在浏览器中忽略该错误，选择"继续浏览此网站"
    缺省登录名：admin，密码：iwg_admin

6、如果要停止请执行：./iwg_asset.sh -stop