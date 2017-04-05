from flask import Flask, abort, request, redirect, url_for, session, escape, g
from flask import send_from_directory, Response, stream_with_context, render_template, jsonify, make_response, current_app
from datetime import timedelta

app = Flask(__name__)
app.secret_key = 'A0Zr98j/3yX R~XHH!jmN]LWX/,?RT'
#app.permanent_session_lifetime = timedelta(minutes=3)

class User:
    def __init__(self, id = 0, name = '', password = '', permission_list = ''):
        self.id             = 0
        self.name           = name
        self.password       = password or ""
        self.permission_list= permission_list

    def value(self):
        return {'id':self.id, 'name':self.name, 'password':self.password, 'permission_list':self.permission_list}
    
    @staticmethod
    def create(kargs):
        return User(**kargs)

@app.before_request
def before_request():
    app.logger.debug('before_request:{0}'.format(request.remote_addr))
    g.u = User(1, 'zte', 'zte123', 'all')

@app.route('/', methods=['GET'])
@app.route('/hello', methods=['GET'])
def hello():
    return "hello, world!"

@app.route('/zte', methods=['GET'])
def zte():
    if 'i' in g:
        i = g.k
        print i
    if 'u' in g:
        return "hello, {0}!".format(g.u.name)
    

@app.route('/login', methods=['POST'])
def login():
    session['i'] = request.form['username']
    return "hello, world!"

u = User.create(User(1, 'zte', 'zte123', 'all').value())
if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8081)