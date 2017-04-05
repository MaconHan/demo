#! python

from app import app

host        = app.config['HOST']
http_type   = app.config['HTTP_TYPE']
ssl_context = None

if http_type and http_type == 'https':
    ssl_context = ('./certificates/server.crt', './certificates/server.key')
    #ssl_context='adhoc'

app.run(ssl_context=ssl_context, threaded=True, **host)
