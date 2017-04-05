#! /bin/bash

#db配置参数
db_name="iwg_asset_db"
db_pwd="zte"
db_port=3306
db_path="./data/db"
db_image="mariadb"

#web配置参数
web_name="iwg_asset_web"
web_port=5000
web_logs_path="./data/logs"
web_data_path="./data/records"
web_image="iwg_asset"

shell_dir=$(realpath "$(dirname "$0")")
cd "${shell_dir}"

#########################################################
help(){
    echo "IWG Asset Management"
    echo ""
    echo "Options:"
    echo "  -build      Build IWG Asset Version"
    echo "  -start      Start IWG Asset"
    echo "  -stop       Stop IWG Asset"
    echo "  -install    Install IWG Asset"
    echo "              Before installation, please ensure that the docker(verson 1.9 or later) has already been installed"
    echo "  -uninstall  Uninstall IWG Asset. This dangerous operation may cause data loss"
    echo "  -upgrade    Upgrade IWG Asset"
}
#########################################################

get_container_addr(){
    local container_name=$1
    echo $(docker inspect ${container_name}|grep -i ipaddress|tail -n1|awk '{print $2}'|egrep -o '([0-9A-Za-z:.]+)')
}

start_db(){
    docker ps -a|grep -i ${db_name}|grep -i exited|awk '{print $1}'|xargs -n1 docker rm -f 2>/dev/null
    db_info=$(docker ps |grep ${db_name})
    if [ -n "$db_info" ]; then
        return 0
    fi

    mkdir -p "${db_path}" 2>/dev/null
    db_path=$(realpath "${db_path}")
    docker run --name ${db_name} \
        -d \
        -p ${db_port}:3306 \
        -v ${db_path}:/var/lib/mysql \
        -v /etc/localtime:/etc/localtime:ro \
        -e MYSQL_ROOT_PASSWORD=${db_pwd} \
        -e LANG=en_US.UTF-8 \
        ${db_image}

    if [ $? -ne 0 ]; then
        return 255
    fi
}

start_web(){
    docker ps -a|grep -i ${web_name}|grep -i exited|awk '{print $1}'|xargs -n1 docker rm -f 2>/dev/null
    web_info=$(docker ps |grep ${web_name})
    if [ -n "$web_info" ]; then
        echo $web_info
        return 0
    fi

    mkdir -p "${web_logs_path}" 2>/dev/null
    mkdir -p "${web_data_path}" 2>/dev/null
    web_logs_path=$(realpath "${web_logs_path}")
    web_data_path=$(realpath "${web_data_path}")
    docker run --name ${web_name} \
        -d \
        -p ${web_port}:5000 \
        -v ${web_logs_path}:/home/iwg_asset/logs \
        -v ${web_data_path}:/home/iwg_asset/data \
        -v /etc/localtime:/etc/localtime:ro \
        -e DEBUG=False \
        -e HTTP_TYPE=https \
        -e DB_HOST=$(get_container_addr ${db_name}) \
        -e DB_PASSWORD=${db_pwd} \
        -e DB_PORT=3306 \
        -e LANG=en_US.UTF-8 \
        ${web_image}

    if [ $? -ne 0 ]; then
        return 255
    fi
}

stop_db(){
    docker stop ${db_name} 2>/dev/null
    docker rm -f ${db_name} 2>/dev/null
    docker ps -a|grep -i ${db_name}|grep -i exited|awk '{print $1}'|xargs -n1 docker rm -f 2>/dev/null
}

stop_web(){
    docker stop ${web_name} 2>/dev/null
    docker rm -f ${web_name} 2>/dev/null
    docker ps -a|grep -i ${web_name}|grep -i exited|awk '{print $1}'|xargs -n1 docker rm -f 2>/dev/null
}

build(){
    echo "build image ..."
    ./build/make_image.sh
    
    echo "build version ..."
    mkdir -p ./version/iwg_asset 2>/dev/null
    cp ./iwg_asset.sh ./readme.txt ./build/images/mariadb.tar ./build/images/iwg_asset.tar ./version/iwg_asset
    
    pushd ./version >/dev/null
    version_file="iwg_asset.tar.gz"
    rm -rf ${version_file}
    tar -zcvf ./${version_file} iwg_asset
    rm -rf iwg_asset
    popd >/dev/null
    
    echo "version dir: ${shell_dir}/version/${version_file}" 
}

install(){
    #检查docker
    docker --version
    if [ $? -ne 0 ]; then
        echo "can't find docker in the machine"
        return 255
    fi
    
    #停止服务
    stop_web
    stop_db

    #导入依赖的镜像文件
    docker load -i ./mariadb.tar
    docker load -i ./iwg_asset.tar

    #初始化数据库
    start_db
    start_web
    echo "Ready to initialize SQL script ..."

    #导入脚本
    for i in $(seq 1 20); do
        sleep 3
        result=$(echo "SHOW DATABASES;" | docker exec -i ${db_name} mysql -p${db_pwd} | grep information_schema)
        if [ -z "${result}" ]; then
            continue
        fi
        
        #从web容器中拷贝sql脚本并执行
        docker cp ${web_name}:/home/iwg_asset/app/sql ./.sql
        cat ./.sql/schema.sql ./.sql/initialize.sql | docker exec -i ${db_name} mysql --default-character-set=utf8 -p${db_pwd}
        rm -rf ./.sql
        break
    done

    stop_web
    stop_db
}

uninstall(){
    stop_web
    stop_db

    rm -rf data
    docker rmi -f mariadb iwg_asset
}

upgrade(){
    stop_web
    #删除旧的web镜像，导入新的web镜像
    docker rmi -f ${web_image}; docker load -i ./iwg_asset.tar

    #启动IWG
    start_db
    start_web
    echo "Ready to upgrade SQL script ..."

    #导入更新脚本
    for i in $(seq 1 5); do
        sleep 3
        result=$(echo "SHOW DATABASES;" | docker exec -i ${db_name} mysql -p${db_pwd} | grep information_schema)
        if [ -z "${result}" ]; then
            continue
        fi
        
        #从web容器中拷贝sql升级脚本并执行
        docker cp ${web_name}:/home/iwg_asset/app/sql ./.sql
        upgrade_file="./.sql/upgrade.sql"
        if [ -f "${upgrade_file}" ]; then
            cat ${upgrade_file} | docker exec -i ${db_name} mysql --default-character-set=utf8 -p${db_pwd}
        fi
        rm -rf ./.sql
        break
    done
}

while [ $# -ne 0 ]
do
    case "$1" in
        "-build")
            shift
            build
            exit 0
            ;;
        "-start")
            shift
            start_db
            start_web
            exit 0
            ;;
        "-stop")
            shift
            stop_web
            stop_db
            exit 0
            ;;
        "-install")
            shift
            install
            exit 0
            ;;
        "-uninstall")
            shift
            uninstall
            exit 0
            ;;
        "-upgrade")
            shift
            upgrade
            exit 0
            ;;
        "--help")
            help
            exit 0
            ;;
        *)
            shift
            ;;
    esac
done

help
