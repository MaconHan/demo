# coding=utf-8
from flask import Flask
from werkzeug.contrib.cache import SimpleCache
from logging.handlers import RotatingFileHandler

cache   = SimpleCache(threshold=256)
app     = Flask(__name__)
app.config.from_object('config')

#日志
if not app.debug:
    import logging
    file_handler = RotatingFileHandler('./logs/iwg.log', 'a', 1 * 1024 * 1024, 4)
    file_handler.setFormatter(logging.Formatter('%(asctime)s %(levelname)s: %(message)s [in %(pathname)s:%(lineno)d]'))
    file_handler.setLevel(logging.DEBUG)

    app.logger.setLevel(logging.DEBUG)
    app.logger.addHandler(file_handler)

    inner_logger = logging.getLogger('werkzeug')
    inner_logger.setLevel(logging.DEBUG)
    inner_logger.addHandler(file_handler)
