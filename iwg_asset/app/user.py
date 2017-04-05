# coding=utf-8
import md5, types

class User:
    def __init__(self, id = 0, name = '', password = '', permission_list = ''):
        self.id             = id
        self.name           = name
        self.password       = password or ""
        self.permission_list= permission_list.lower()

    @staticmethod
    def make_password_md5(name, password):
        key = "{0}:{1}".format(name.lower(), password)
        m = md5.new()
        m.update(key)
        return m.hexdigest()

    def password_md5(self):
        return User.make_password_md5(self.name, self.password)

    def check_password(self, pwd_md5):
        pwd_md5 = pwd_md5 or ""
        if self.password_md5() == pwd_md5:
            return True
        return False

    def validate(self, op):
        if self.permission_list == 'all':
            return True
        
        if type(op) is types.StringType:
            op = op.lower()
            return self.permission_list.find(op) >= 0
        elif type(op) is types.ListType:
            for o in op:
                o = o.lower()
                if self.permission_list.find(0) >= 0:
                    return True
        
        return False

    def value(self):
        return {'id':self.id, 'name':self.name, 'password':self.password, 'permission_list':self.permission_list}

    @staticmethod
    def create(kargs):
        return User(**kargs)

