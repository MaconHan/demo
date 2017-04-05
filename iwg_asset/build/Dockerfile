FROM centos
MAINTAINER han.yamin

ENV LANG=en_US.UTF-8

#拷贝安装包
ENV Path=/home/iwg_asset

COPY ./app ${Path}/app
COPY ./certificates ${Path}/certificates
COPY ./*.sh ${Path}/
COPY ./*.py ${Path}/
COPY ./build/install ${Path}/install
RUN mkdir -p ${Path}/logs
RUN mkdir -p ${Path}/data

#安装包路径
ENV Packages_Path=${Path}/install

#安装python包
RUN rpm -ivh --nodeps --force ${Packages_Path}/python*.rpm

#安装pip
WORKDIR ${Packages_Path}
RUN python ez_setup.py
RUN python get-pip.py -U --no-index -f ./
#安装软件包
RUN pip install --no-index -f ./ flask mysql-connector
ADD ./build/install/Crypto.tar /usr/lib/python2.7/site-packages

#暴露web服务
EXPOSE 5000

#启动程序
WORKDIR ${Path}
ENTRYPOINT ["python", "web.py"]
