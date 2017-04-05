# coding=utf-8

import json, re, types, datetime, os, md5, uuid
import mysql.connector
from app import app, cache, user, gweui, tools
from app.errors import *
from flask import request, session, g, current_app
from flask import redirect, url_for, escape, send_from_directory, stream_with_context, render_template, jsonify, make_response
from flask import Flask, abort, Response

DB_CONFIG       = app.config['MYSQL_DB']
GWEUI_MAX_SIZE  = 500
AUTH_IGNORE_URL = ('/', '/login', '/favicon.ico')

def db_connect():
    g.db_conn   = mysql.connector.connect(**DB_CONFIG)
    g.db_cursor = g.db_conn.cursor()

@app.before_request
def before_request():
    g.db_conn, g.db_cursor = None, None

    path = request.path
    if path in AUTH_IGNORE_URL or path.startswith('/static/'):
        pass
    #校验会话
    elif not session or 'user' not in session:
        app.logger.error('invaild session')
        #session['user'] = user.User().value()
        return redirect('/')
    
    if session and 'user' in session:
        g.user = user.User.create(session['user'])

@app.teardown_request
def teardown_request(*args):
    if g.db_cursor:
        g.db_cursor.close()
    if g.db_conn:
        g.db_conn.close()
    g.db_conn, g.db_cursor = None, None

@app.route('/', methods=['GET'])
def index():
    if 'user' in g:
        return redirect('/main')
    return render_template('login.html')

@app.route('/favicon.ico', methods=['GET'])
def favicon():
    return send_from_directory(os.path.join(app.root_path, 'static'), 'favicon.ico', mimetype='image/vnd.microsoft.icon')

@app.route('/main', methods=["GET"])
def main():
    u = g.user
    superman = (u.name == 'admin')
    return render_template('main.html', username=u.name, superman=superman)

@app.route('/login', methods=['POST'])
def login():
    username = request.form['username'].lower().strip()
    password = request.form['password']
    u = user.User(name=username, password=password)

    result = Result()
    try:
        db_connect()

        stmt = 'select sql_cache id, pwd_md5, permission_list from tb_user where name = %s and status != -1'
        g.db_cursor.execute(stmt, (username,))
        value = g.db_cursor.fetchall()
        if len(value) == 1 and (not value[0][1] or u.check_password(value[0][1])):
            u.id                = value[0][0]
            u.permission_list   = value[0][2]
            session['user']     = u.value()
            app.logger.debug("login: user={0}-{1}".format(u.id, u.name))
        else:
            app.logger.error("login: user={0}, user is not exist or password is incorrect".format(u.name))
            result.error(ERROR_LoginFail)
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/logout', methods=["GET"])
def logout():
    app.logger.debug("logout: user={0}".format(g.user.name))
    for key in session.keys():
        session.pop(key, None)
    return redirect('/')

@app.route('/user/add', methods=['POST'])
def add_user():
    result = Result()

    try:
        if g.user.name != 'admin':
            result.error(ERROR_NoAuth)
        if not request.data:
            raise Error(ERROR_NoData)

        u = json.loads(str(request.data))
        name            = u.get('name', '').lower().strip()
        password        = u.get('password', '')
        permission_list = u.get('permission_list', 'all')
        if len(name) == 0:
            raise Error(ERROR_NoUser)

        db_connect()
        u = user.User(0, name, password, permission_list)
        stmt = '''select id from tb_user where name = %s and status != -1'''
        g.db_cursor.execute(stmt, (u.name, ))
        data = g.db_cursor.fetchall()
        if len(data) != 0:
            raise Error(ERROR_UserExist)

        stmt = '''insert into tb_user (name, description, pwd_md5, permission_list, status) 
                  values (%s, %s, %s, %s, 1)'''
        g.db_cursor.execute(stmt, (u.name, u.name, u.password_md5(), u.permission_list))
        user_id  = g.db_cursor.lastrowid
        g.db_conn.commit()

        app.logger.info("add user: user={0}-{1}".format(user_id, name))
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Error as e:
        result.error(e.error, *e.args)
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/user/delete/<username>', methods=['DELETE'])
def delete_user(username):
    result = Result()
    username = username.lower().strip()
    if g.user.name != 'admin' or username == 'admin':
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    try:
        db_connect()

        stmt = '''update tb_user set status = -1 where name=%s and status != -1'''
        g.db_cursor.execute(stmt, (username, ))
        g.db_conn.commit()

        app.logger.info("delete user: user={0}".format(username))
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/user/change/password', methods=["PUT"])
@app.route('/user/change/password/<password>', methods=["PUT"])
def change_password(password = None):
    result = Result()

    try:
        username = g.user.name
        if password is None:
            if g.user.name != 'admin':
                raise Error(ERROR_NoAuth)
            if not request.data:
                raise Error(ERROR_NoData)

            u = json.loads(str(request.data))
            username = u.get('name', '').lower().strip()
            password = u.get('password', '')
        if len(username) == 0:
            raise Error(ERROR_NoUser)

        db_connect()
        stmt = '''select id from tb_user where name = %s and status != -1'''
        g.db_cursor.execute(stmt, (username, ))
        data = g.db_cursor.fetchall()
        if len(data) == 0:
            raise Error(ERROR_NoUser)

        user_id = data[0][0]
        pwd_md5 = user.User.make_password_md5(username, password)
        stmt = '''update tb_user set pwd_md5 = %s where id = %s'''
        g.db_cursor.execute(stmt, (pwd_md5, user_id))
        g.db_conn.commit()

        if username == g.user.name:
            g.user.password = password
        app.logger.info("change password: user={0}".format(username))
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Error as e:
        result.error(e.error, *e.args)
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/user/list', methods=['GET'])
def list_user():
    result = Result()
    if g.user.name != 'admin':
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    try:
        db_connect()

        stmt = '''select id, name, description, permission_list from tb_user where status != -1'''
        g.db_cursor.execute(stmt)
        
        data = g.db_cursor.fetchall()
        result.data = [{'id':               i[0], 
                        'name':             i[1], 
                        'description':      i[2], 
                        'permission_list':  i[3]} 
                        for i in data]
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/gweui/create', methods=["POST"])
def create_gweui():
    result  = Result()
    remote  = request.remote_addr
    userid  = g.user.id
    username= g.user.name
    size    = 20

    if not g.user.validate('create'):
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    try:
        condition   = str(request.data)
        condition   = json.loads(condition)
        gw_eui      = gweui.GWEUI(**condition)
        size        = int(condition['size'] or size)

        if size > GWEUI_MAX_SIZE:
            raise Error(ERROR_OverCap, GWEUI_MAX_SIZE)
    except Error as e:
        result.error(e.error, *e.args)
        return jsonify(result.value())
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))
        return jsonify(result.value())

    serial  = 0
    space   = 0
    session.pop('current_log', None)

    try:
        db_connect()

        #查询最近使用的生产序列号
        gweui_field_value = {
            'a':gw_eui.oui, 
            'b':gw_eui.rate_range, 
            'c':gw_eui.power_type, 
            'd':gw_eui.data_back, 
            'e':gw_eui.factory 
        }
        stmt = '''select serial, space from tb_gweui_usage 
                  where oui=%(a)s and rate_range=%(b)s and power_type=%(c)s and data_back=%(d)s and factory=%(e)s
                  for update'''
        g.db_cursor.execute(stmt, gweui_field_value)
        value = g.db_cursor.fetchall()
        if len(value) > 0:
            serial  = int(value[0][0], 16)
            space   = int(value[0][1])
        else:
            space   = gw_eui.space()

        if space < size:
            raise Error(ERROR_NoSpace, space)
        gweui_field_value['f'] = '%05X' % (serial + size)
        gweui_field_value['g'] = space - size

        gw_eui.serial   = serial
        last_gw_eui = gw_eui.value()
        gweui_list  = gweui.make_gweui_secret(last_gw_eui, size)
        create_time = datetime.datetime.now()

        try:
            #更新gweui使用记录表
            stmt0 = ''' replace into tb_gweui_usage (oui, rate_range, power_type, data_back, factory, serial, space) 
                        values (%(a)s, %(b)s, %(c)s, %(d)s, %(e)s, %(f)s, %(g)s)'''
            g.db_cursor.execute(stmt0, gweui_field_value)

            #记录日志
            stmt1 = '''insert into tb_logs (serial, operator, operate_time, operate_type, message) values (%s, %s, %s, %s, %s)'''
            log_serial  = tools.create_log_serial(create_time, userid)
            log_message = "gweui=%016X\nsize=%d\nclient=%s" % (last_gw_eui, size, remote)
            g.db_cursor.execute(stmt1, (log_serial, userid, create_time, 'create', log_message))
            log_id  = g.db_cursor.lastrowid

            #提交产生gweui的secret_key
            status  = 1#-1:已经启用, 0:未分配, 1:已分配
            values  = [(gweui_, key_, userid, create_time, status, log_id) for gweui_, key_ in gweui_list]
            stmt2   = '''insert into tb_gweui_secret_key (gweui, secret_key, creator, create_time, status, operate_serial_list) 
                         values (%s, %s, %s, %s, %s, %s)'''
            g.db_cursor.executemany(stmt2, values)

            #提交
            g.db_conn.commit()

            result.data             = [(gweui_, key_[0:-8] + '*' * 8) for gweui_, key_ in gweui_list]
            session['current_log']  = (log_id, log_serial)
            app.logger.info("user({0}:{1}) create gweui, log_id is {2}, log_serial is {3}".format(userid, username, log_id, log_serial))

            #记录到文件中
            try:
                if not os.path.exists('./data'):
                    os.mkdir('./data')

                create_time = create_time.strftime('%Y-%m-%d %H:%M:%S')
                data = ['%s,%s,%s,%s(%d)' % (gweui_, key_, create_time, username, userid) for gweui_, key_ in gweui_list]
                data = '\n'.join(data)
                with open('./data/gweui.txt', 'a') as f:
                    f.write(data)
                    f.write('\n')
            except Exception as e:
                app.logger.error('Failed to create GWEUI information, %s' % str(e))
            finally:
                pass
        except mysql.connector.Error as e:
            g.db_conn.rollback()
            app.logger.error("{0}".format(e))
            result.error(ERROR_DBFail, str(e))
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Error as e:
        result.error(e.error, *e.args)
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/gweui/export', methods=["GET", "POST"])
@app.route('/gweui/export/<type>', methods=["GET", "POST"])
def export_gweui(type = 'encrypt'):
    result  = Result()
    if not g.user.validate(['create', 'export']):
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    gweui_list = []
    try:
        if request.method == 'POST':
            output_file = 'gweui_secret_key'
            data = request.data
            data = json.loads(data)

            db_connect()
            for gweui_ in set(data):
                stmt = 'select secret_key from tb_gweui_secret_key where gweui=%s'
                g.db_cursor.execute(stmt, (gweui_,))
                value = g.db_cursor.fetchall()
                if len(value) == 1:
                    gweui_list.append((gweui_, value[0][0]))
        elif request.method == 'GET':
            if 'current_log' not in session:
                raise Error(ERROR_NoData)
            log_id, log_serial = session['current_log']
            output_file = log_serial

            db_connect()
            gweui.query_gweui_bylog(g.db_cursor, log_id)
            data = g.db_cursor.fetchall()

            gweui_index     = g.db_cursor.column_names.index('gweui')
            secret_key_index= g.db_cursor.column_names.index('secret_key')
            gweui_list = [(i[gweui_index], i[secret_key_index]) for i in data]
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Error as e:
        result.error(e.error, *e.args)
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    if result.err_code != 0:
        return jsonify(result.value())
    if not gweui_list:
        result.error(ERROR_NoData)
        return jsonify(result.value())
        #return current_app.response_class(
        #        (unicode(result), '\n'),
        #         status=403,
        #         mimetype=current_app.config['JSONIFY_MIMETYPE']
        #        )

    data = {gweui_ : key_ for gweui_, key_ in gweui_list}
    data = json.dumps(data, indent = 2)

    if type not in ('text', 'txt', 'json') :
        data        = tools.aes_encrypt(data)
        output_file = '%s.dat' % (output_file,)
    else:
        output_file = '%s.%s' % (output_file, type)

    #将待下载的文件放到cache中，待/download/filename执行下载
    m = md5.new()
    m.update(uuid.uuid4().hex)
    cache_file = m.hexdigest()
    cache_file = cache_file[:16]
    cache_data = (output_file, data)
    cache.set(cache_file, cache_data, timeout = 5)

    #返回待下载文件名
    result.data = {'filename':cache_file}
    return jsonify(result.value())

@app.route('/download/<filename>', methods=["GET"])
def download(filename):
    if not cache.has(filename):
        cache.delete(filename)
        abort(404)

    response = None
    try:
        output_file, data = cache.get(filename)
        response = make_response(data)
        response.headers["Content-Disposition"] = "attachment;filename={0}".format(output_file)
    except Exception as e:
        app.logger.error("failed to download: %s" % e)
    finally:
        cache.delete(filename)
        if not response:
            abort(500)

    return response

@app.route('/gweui/define', methods=["GET"])
def get_gweui_resource_definition():
    result  = Result()
    try:
        result.data = gweui.get_gweui_dim()
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    result = unicode(result)
    return current_app.response_class(
        (result, '\n'),
        mimetype=current_app.config['JSONIFY_MIMETYPE']
    )

@app.route('/gweui/get/<gweui>', methods=["GET"])
def get_gweui_info(gweui = ''):
    result      = Result()

    if not g.user.validate(['create', 'export', 'query']):
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    try:
        db_connect()

        stmt = 'select gweui, secret_key, create_time, status from tb_gweui_secret_key where gweui=%s'
        g.db_cursor.execute(stmt, (gweui,))

        data = g.db_cursor.fetchall()
        if len(data) == 1:
            data = data[0]
            result.data = { 'gweui':data[0], 
                            'secret_key':(str(data[1])[0:-8] + '*' * 8), 
                            'create_time':data[2].strftime("%Y-%m-%d %H:%M:%S"),
                            'status':data[3]}
        else:
            result.error(ERROR_NoExist)
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))

    return jsonify(result.value())

@app.route('/gweui/put/<int:status>', methods=["PUT"])
def put_gweui_info(status):
    result  = Result()
    remote  = request.remote_addr
    userid  = g.user.id

    if not g.user.validate('change'):
        result.error(ERROR_NoAuth)
        return jsonify(result.value())

    try:
        data = request.data
        data = json.loads(data)
        if type(data) is not types.ListType or len(data) == 0:
            raise Error(ERROR_ExecFail, "无效数据")

        status  = int(status)
        if status not in (-1, 0, 1):
            raise Exception, ("status '{0}' undefined".format(status))

        operate_time= datetime.datetime.now()
        log_serial  = tools.create_log_serial(operate_time, userid)
        stmt1       = '''insert into tb_logs (serial, operator, operate_time, operate_type, message) values (%s, %s, %s, %s, %s)'''
        stmt2       = '''update tb_gweui_secret_key 
                         set status=%s, operate_serial_list=concat_ws(',', operate_serial_list, %s) 
                         where gweui=%s'''
        
        db_connect()
        try:
            for gweui in data:
                #记录日志
                message = "gweui={0}\nstatus={1}\nclient={2}".format(gweui, status, remote)
                g.db_cursor.execute(stmt1, (log_serial, userid, operate_time, 'change', message))
                log_id = g.db_cursor.lastrowid

                #修改gweui的状态，并记录操作流水号
                g.db_cursor.execute(stmt2, (status, log_id, gweui))

            g.db_conn.commit()
        except (mysql.connector.Error, Exception) as e:
            g.db_conn.rollback()
            result.error(ERROR_DBFail, str(e))
    except mysql.connector.Error as e:
        result.error(ERROR_DBFail, str(e))
    except Error as e:
        result.error(e.error, *e.args)
    except Exception as e:
        result.error(ERROR_ExecFail, str(e))

    return jsonify(result.value())

@app.route('/logs/get', methods=["GET"])
@app.route('/logs/get/<int:rows>', methods=["GET"])
@app.route('/logs/get/<int:offset>/<int:rows>', methods=["GET"])
def get_logs(offset = 0, rows = 20):
    result  = Result()
    userid  = g.user.id

    try:
        db_connect()

        #查询日志
        offset  = int(offset)
        rows    = int(rows)
        if rows == -1:
            rows = 65536
        
        stmt0 = ''' select a.id, serial, b.name, operate_time, operate_type, message 
                    from tb_logs as a left join tb_user as b on a.operator = b.id 
                    where a.operator=%s 
                    order by operate_time desc 
                    limit %s, %s'''
        g.db_cursor.execute(stmt0, (userid, offset, rows))
        data = g.db_cursor.fetchall()
        result.data = [{'id':           i[0], 
                        'serial':       i[1], 
                        'operator':     i[2], 
                        'operate_time': i[3].strftime("%Y-%m-%d %H:%M:%S"), 
                        'operate_type': i[4], 
                        'message':      i[5]} 
                        for i in data]
    except (mysql.connector.Error, Exception) as e:
        result.error(ERROR_DBFail, str(e))

    return jsonify(result.value())

@app.route('/gweui/get/log_id/<int:log_id>', methods=['GET'])
def get_gweui_bylogs(log_id = -1):
    result = Result()
    log_id = int(log_id)

    try:
        db_connect()

        gweui.query_gweui_bylog(g.db_cursor, log_id)
        data = g.db_cursor.fetchall()
        gweui_index     = g.db_cursor.column_names.index('gweui')
        secret_key_index= g.db_cursor.column_names.index('secret_key')
        status_index    = g.db_cursor.column_names.index('status')
        result.data = [
                        {
                            'gweui':i[gweui_index], 
                            'secret_key':(str(i[secret_key_index])[0:-8] + '*' * 8), 
                            'status':i[status_index]
                        } for i in data
                      ]
    except Error as e:
        result.error(e.error, *e.args)
    except (mysql.connector.Error, Exception) as e:
        result.error(ERROR_DBFail, str(e))

    return jsonify(result.value())
