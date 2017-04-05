#! /bin/bash

#全局环境变量
shell_dir=$(realpath "$(dirname "$0")")
cd ${shell_dir}

root_dir="${shell_dir}/.."
output_dir="${shell_dir}/images"
image_name="iwg_asset"

centos=$(docker ps |grep centos)
if [ -z "$centos" ]; then
    docker load -i ./images/centos.tar
fi

cp -f ./Dockerfile ${root_dir}

cd ${root_dir}
docker rmi -f ${image_name} 2>/dev/null
docker build -t ${image_name} ./
ret=$?
rm -f Dockerfile

if [ $ret -eq 0 ] && [ -n "$output_dir" ] && [ -d "$output_dir" ]; then
    docker save ${image_name}:latest > "${output_dir}/${image_name}.tar"
fi
