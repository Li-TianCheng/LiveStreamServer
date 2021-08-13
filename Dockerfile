FROM modules:v1.0
MAINTAINER Li-TianCheng "li_tiancheng666@163.com"
COPY . /LiveStreamServer/
WORKDIR /LiveStreamServer
RUN cd scripts && bash ./build.sh && cd ..
CMD ["./output/bin/LiveStreamServer", "-c", "./output/config/config.json"]