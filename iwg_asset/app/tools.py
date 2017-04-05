from Crypto.Cipher import AES
from app import app
import datetime

_AES_KEY = app.config["AES_KEY"]
_AES_VI  = b'0000000000000000'
def aes_encrypt(data):
    aes     = AES.new(_AES_KEY, AES.MODE_CBC, _AES_VI)
    data    = data + ('\0' * (16 - (len(data) % 16)))
    data    = aes.encrypt(data)
    return data

def aes_decrypt(data):
    aes     = AES.new(_AES_KEY, AES.MODE_CBC, _AES_VI)
    data    = aes.decrypt(data)
    data    = data.rstrip('\0')
    return data


def create_log_serial(time, user_id):
    delta = time - datetime.datetime(time.year, time.month, time.day)
    milliseconds = delta.seconds * 1000 + time.microsecond / 1000
    return '%s%05d%08d' % (time.strftime("%Y%m%d"), user_id, milliseconds)
