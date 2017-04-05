# coding=utf-8

import mysql.connector
import json
from Crypto.Cipher import AES

db_config = {'host':'10.47.73.123',
    'user':'root',
    'password':'zte',
    'port':3306 ,
    'database':'iwg_asset_management',
    'charset':'utf8'}

def mysql_query():
    try:
        conn = mysql.connector.connect(**db_config)
        cursor = conn.cursor()
        
        username    = 'admin'
        query_user  = "select * from tb_gweui_usage; select * from tb_logs limit 5"
        iter = cursor.execute(query_user, multi=True)
        for i in iter:
            try:
                data = i.fetchall()
                print data
            except (mysql.connector.Error, Exception) as e:
                print str(e)
            finally:
                pass
    except (mysql.connector.Error, Exception) as e:
        print str(e)
    finally:
        cursor.close()
        conn.close()


def create_gweui():
    d = {
        "oui":"004A77", 
        "rate_range":"01",
        "power_type":"1",
        "data_back":"2",
        "factory":"1",
        "size":"10"
    }

    gw = app.gweui.GWEUI(**d)
    gweui_list = app.gweui.make_gweui_secret(gw.value(), 10)
    data = json.dumps(gweui_list)
    print data

def decrypt_gweuid_secret_key():
    data = ""
    with open('''F:\software\gweui_secret_key.dat''', 'rb') as f:
        data = f.read()
    
    data = app.tools.aes_decrypt(data)
    print data
    return data

class Result:
    def __init__(self, err_code = 0, err_msg = '', data = None):
        self.err_code   = err_code
        self.err_msg    = err_msg
        self.data       = data

    def error(self, error, *args):
        self.err_code = error[0]
        self.err_msg  = str(error[1]).format(*args)

    def value(self):
        return {
            'err_code': self.err_code,
            'err_msg':  self.err_msg,
            'data':     self.data
        }

    def __str__(self):
        s = json.dumps(self.value(), indent=2, ensure_ascii=False, encoding='utf-8')
        return s.decode('utf-8')

if __name__ == "__main__":
    mysql_query()