# coding=utf-8

import md5, uuid, re
import mysql.connector
import math
from app import app, cache
from app.errors import *

def get_gweui_dim():
    cache_key_resource = 'resource'
    data = cache.get(cache_key_resource)
    if data is not None:
        return data
    
    resource = {}
    error    = None
    try:
        conn, cursor = None, None
        conn    = mysql.connector.connect(**app.config['MYSQL_DB'])
        cursor  = conn.cursor()

        stmt = 'select sql_cache name, value, description from tb_gweui_resource_definition'
        cursor.execute(stmt)
        data = cursor.fetchall()
        for i in data:
            name        = i[0]
            value       = i[1]
            description = i[2]
            d = {'value':value, 'description':description}

            name = name.lower()
            if name in resource:
                resource[name].append(d)
            else:
                resource[name] = [d]
        
        cache.set(cache_key_resource, resource, timeout = 15 * 60)
        app.logger.debug('gweui dimension cache is expire, and recache')
    except mysql.connector.Error as e:
        error = str(e)
    finally:
        cursor and cursor.close()
        conn and conn.close()
    
    if error is not None:
        raise Exception, error
    return resource


#oui    24bit
#serial 40bit
def make_gweui_secret(gweui, size = 20):
    m = md5.new()
    m.update("%016X" % gweui)
    gweui_secret_list = []

    for i in range(size):
        m.update(uuid.uuid4().hex)
        secret_key  = m.hexdigest()
        gweui_secret= ("%016X" % gweui, secret_key)
        gweui_secret_list.append(gweui_secret)
        gweui += 1

    return gweui_secret_list

def query_gweui_bylog(db_cursor, log_id):
    #查询日志详细信息
    stmt = '''select operate_type, message from tb_logs where id=%s'''
    db_cursor.execute(stmt, (log_id,))
    data = db_cursor.fetchall()
    if len(data) != 1:
        raise Error(ERROR_NoLog)

    operate_type= data[0][0]
    message     = data[0][1]

    gweui   = re.findall(r'gweui=([\w]*)', message, re.I|re.M)
    gweui   = gweui[0] if gweui else ''

    rows    = re.findall(r'size=([\d]*)', message, re.I|re.M)
    rows    = int(rows[0]) if rows else 1

    #根据log_id值，查询对应的gweui和secret_key
    stmt = '''select id, gweui, secret_key, status from tb_gweui_secret_key 
            where gweui >= %s and find_in_set(%s, operate_serial_list)
            limit %s'''
    return db_cursor.execute(stmt, (gweui, log_id, rows))

class GWEUI:
    OUI         = 'oui'
    RATE_RANGE  = 'rate_range'
    POWER_TYPE  = 'power_type'
    DATA_BACK   = 'data_back'
    FACTORY     = 'factory'

    def __init__(self, **kargs):
        self.oui        = kargs.get(GWEUI.OUI, None)
        self.rate_range = kargs.get(GWEUI.RATE_RANGE, None)
        self.power_type = kargs.get(GWEUI.POWER_TYPE, None)
        self.data_back  = kargs.get(GWEUI.DATA_BACK, None)
        self.factory    = kargs.get(GWEUI.FACTORY, None)
        self.serial     = 0

        def find_dim_field(data, value):
            for i in data:
                if i['value'] == value:
                    return True
            return False

        dim = get_gweui_dim()
        l   = [ (GWEUI.OUI, self.oui), 
                (GWEUI.RATE_RANGE, self.rate_range), 
                (GWEUI.POWER_TYPE, self.power_type), 
                (GWEUI.DATA_BACK, self.data_back), 
                (GWEUI.FACTORY, self.factory)
              ]
        for i in l:
            if i[0] not in dim or not find_dim_field(dim[i[0]], i[1]):
                raise Exception, "{0} '{1}' is not defined".format(i[0], i[1])

    def value(self):
        oui_        = long(self.oui, 16)
        rate_range_ = long(self.rate_range, 16)
        power_type_ = long(self.power_type, 16)
        data_back_  = long(self.data_back, 16)
        factory_    = long(self.factory, 16)

        eui = (oui_ << 40)
        eui = eui + (rate_range_ << 32)
        eui = eui + (power_type_ << 28)
        eui = eui + (data_back_ << 24)
        eui = eui + (factory_ << 20)
        eui = eui + (self.serial & 0x00000000000FFFFF)
        return eui
    
    def capacity(self):
        return int(2**20)

    def space(self):
        return self.capacity() - self.serial;