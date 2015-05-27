# [links](https://github.com/coordcn/links)
http server base on lua + libuv
项目灵感来自[fibjs](http://fibjs.org/)和[openresty](http://openresty.org/)。
实现目标：通过对coroutine包装实现同步代码编写异步程序，lua代码实现[形式同步]，c代码实现[实质异步]。
开发模式：需求驱动

##第一阶段工作：http1.1 预计三个月完成

###模块
####system

system.cpuinfo()

system.meminfo()

system.loadavg()

system.hrtime()

system.uptime()

####process

process.fork()

process.exec()

process.cwd()

process.exit()

process.abort()

process.kill()

process.uptime()

####dns

dns.resolve4(hostname)

dns.resolve6(hostname)

####tcp 进行中

####http 待实现

####websocket 待实现

以下模块采用迭代开发模式，逐步完善

####https 待实现

####redis

####mysql

####mongodb

####sequoiadb

##第二阶段工作：测试 预计两个月完成

####单元测试

####性能测试

##第三阶段工作：实例 结合实际情况开展项目，通过项目开发过程中出现的问题，形成需求，驱动links开发

####社区

####聊天

####游戏

##第四阶段工作：http2.0

####spdy3.0 看http2.0发展情况，如果http2.0推广的迅速就跳过这个阶段

####http2.0





