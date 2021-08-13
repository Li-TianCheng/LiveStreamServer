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
   >回源:http://localhost:7070/v1/stream/relay_source?rtmp://localhost:1935/vhost/app/stream
   > 
   >转推:http://localhost:7070/v1/stream/relay_sink?http://localhost:8070/vhost/app/stream
   > 
   >服务重启:http://localhost:7070/v1/server/restart
   > 
   >服务关闭:http://localhost:7070/v1/server/shutdown
   > 
   >服务优雅关闭:http://localhost:7070/v1/server/stop
   > 
   >服务平滑升级:http://localhost:7070/v1/server/update
   >
   >服务资源占用:http://localhost:7070/server/status
3. 支持网页播放flv
   >http://localhost:8080 进入引导页
4. 支持vhost app stream 三级配置
5. 支持配置转推回源
   >转推配置:在live_stream_server配置中添加如下配置
   ```json
   "relay_sink": {
      "vhost1": {
        "app1": [
          {
            "type": "http",
            "host": "localhost:8070"
          },
          {
            "type": "rtmp",
            "host": "localhost:1935"
          }
        ]
      }
    }
   ```
   >回源配置:在live_stream_server配置中添加如下配置
   ```json
   "relay_source": {
      "vhost1": {
        "app1": [
          {
            "type": "http",
            "host": "localhost:8070"
          },
          {
            "type": "rtmp",
            "host": "localhost:1935"
          }
        ]
      }
    }
   ```
## TODO
1. 性能对比 
2. 性能优化
