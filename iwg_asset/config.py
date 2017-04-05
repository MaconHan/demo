# coding=utf-8

import os

#flask 系统配置
DEBUG               = (os.environ.get('DEBUG', '') != 'False')
HOST                = {'host':'0.0.0.0', 'port':5000}
SECRET_KEY          = '53a2e264-ff9e-11e6-97c8-0022937192b6'
LOGGER_NAME         = 'claa_iwg'
SESSION_COOKIE_NAME = 'claa_iwg'
JSON_AS_ASCII       = False
PERMANENT_SESSION_LIFETIME  = 15 * 60 * 60
SEND_FILE_MAX_AGE_DEFAULT   = 15 * 60

#http协议类型
HTTP_TYPE = os.environ.get('HTTP_TYPE', None) or 'https' #'https', 'http'

#导出文件秘钥
AES_KEY = '626ddefc34414fafbc43ce50cb9085ef'

#mysql数据库
MYSQL_DB = {
    'host'      :os.environ.get('DB_HOST', None) or '10.47.73.161',
    'user'      :'root',
    'password'  :os.environ.get('DB_PASSWORD', None) or 'zte',
    'port'      :int(os.environ.get('DB_PORT', None) or 3306),
    'database'  :'iwg_asset_management',
    'charset'   :'utf8'
}
