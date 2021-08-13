#! /bin/bash

echo "=================================创建基础镜像================================="
docker build -t modules:v1.0 ../modules/
echo "=================================创建应用镜像================================="
docker build -t live_stream_server:v1.0 ../
echo "=================================开始运行================================="
docker run -d --rm -p 8080:8080 -p 1935:1935 -p 7070:7070 -p 8070:8070 live_stream_server:v1.0