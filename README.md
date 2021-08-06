# LiveStreamServer
## 直播流媒体服务器
1. 支持rtmp, http-flv协议
   >rtmp推拉流url:rtmp://localhost:1935/vhost/app/stream
   > 
   >http-flv推拉流url:http://localhost:8070/vhost/app/stream
2. 支持简易api
   >查看流状态:http://localhost:7070/v1/stream/status
   >
   >查看流名:http://localhost:7070/v1/stream/name
   > 
   >服务重启:http://localhost:7070/v1/server/restart
   > 
   >服务关闭:http://localhost:7070/v1/server/stop
   > 
   >服务资源占用:http://localhost:7070/server/status
3. 支持网页播放flv
   >http://localhost:8080 进入引导页
4. 支持vhost app stream 三级配置
## TODO
1. 支持配置转推回源
2. 支持api转推回源
3. 支持平滑升级
4. 性能优化
