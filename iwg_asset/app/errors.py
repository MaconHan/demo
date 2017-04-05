# coding=utf-8
import json, types

ERROR_OK        = (0, "")
ERROR_NoAuth    = (1, "你没有该操作权限")
ERROR_LoginFail = (2, "登陆错误.密码错误或者该用户不存在")
ERROR_NoLogin   = (3, "未登陆")
ERROR_UserExist = (4, "相同的用户已经存在")
ERROR_NoUser    = (5, "用户不存在")

ERROR_DBFail    = (10, "数据库执行错误:{0}")
ERROR_OverCap   = (11, "超过允许最大创建数:{0}")
ERROR_NoData    = (12, "数据不存在")
ERROR_NoExist   = (13, "GWEUI不存在")
ERROR_Deprecated= (14, "GWEUI被弃用,禁止修改其状态")
ERROR_NoChange  = (15, "GWEUI的状态未被修改")
ERROR_ExecFail  = (16, "执行失败:{0}")
ERROR_NoSpace   = (17, "GWEUI没有空闲可使用.当前可用空闲数:{0}")
ERROR_NoLog     = (18, "未找到相关的操作日志")

class Error(Exception):
    def __init__(self, error, *args):
        self.error  = error
        self.args   = args

    def code(self):
        return self.error[0]

    def message(self):
        return self.error[1]

class Result:
    def __init__(self, err_code = 0, err_msg = '', data = None):
        self.err_code   = err_code
        self.err_msg    = err_msg
        self.data       = data

    def error(self, error, *args):
        self.err_code, self.err_msg = error
        if len(args) > 0:
            self.err_msg  = str(self.err_msg).format(*args)

    def value(self):
        return {
            'err_code': self.err_code,
            'err_msg':  self.err_msg,
            'data':     self.data
        }

    def __str__(self):
        s = json.dumps(self.value(), indent=2, ensure_ascii=False, encoding='utf-8')
        if type(s) is types.UnicodeType:
            return s
        return s.decode('utf-8')

