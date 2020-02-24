
[root@six huangzhihui]# ls /usr/include/netinet/
ether.h  if_ether.h  if_tr.h  in.h        ip6.h  ip_icmp.h  udp.h
icmp6.h  if_fddi.h   igmp.h   in_systm.h  ip.h   tcp.h

网络协议结构体定义


// i386 is little_endian.   
#ifndef LITTLE_ENDIAN   
#define LITTLE_ENDIAN   (1)   //BYTE ORDER   
#else   
#error Redefine LITTLE_ORDER   
#endif   
//Mac头部，总长度14字节   
typedef struct _eth_hdr  
{  
    unsigned char dstmac[6]; //目标mac地址   
    unsigned char srcmac[6]; //源mac地址   
    unsigned short eth_type; //以太网类型   
}eth_hdr;  
//IP头部，总长度20字节   
typedef struct _ip_hdr  
{  
    #if LITTLE_ENDIAN   
    unsigned char ihl:4;     //首部长度   
    unsigned char version:4, //版本    
    #else   
    unsigned char version:4, //版本   
    unsigned char ihl:4;     //首部长度   
    #endif   
    unsigned char tos;       //服务类型   
    unsigned short tot_len;  //总长度   
    unsigned short id;       //标志   
    unsigned short frag_off; //分片偏移   
    unsigned char ttl;       //生存时间   
    unsigned char protocol;  //协议   
    unsigned short chk_sum;  //检验和   
    struct in_addr srcaddr;  //源IP地址   
    struct in_addr dstaddr;  //目的IP地址   
}ip_hdr;  
//TCP头部，总长度20字节   
typedef struct _tcp_hdr  
{  
    unsigned short src_port;    //源端口号   
    unsigned short dst_port;    //目的端口号   
    unsigned int seq_no;        //序列号   
    unsigned int ack_no;        //确认号   
    #if LITTLE_ENDIAN   
    unsigned char reserved_1:4; //保留6位中的4位首部长度   
    unsigned char thl:4;        //tcp头部长度   
    unsigned char flag:6;       //6位标志   
    unsigned char reseverd_2:2; //保留6位中的2位   
    #else   
    unsigned char thl:4;        //tcp头部长度   
    unsigned char reserved_1:4; //保留6位中的4位首部长度   
    unsigned char reseverd_2:2; //保留6位中的2位   
    unsigned char flag:6;       //6位标志    
    #endif   
    unsigned short wnd_size;    //16位窗口大小   
    unsigned short chk_sum;     //16位TCP检验和   
    unsigned short urgt_p;      //16为紧急指针   
}tcp_hdr;  
//UDP头部，总长度8字节   
typedef struct _udp_hdr  
{  
    unsigned short src_port; //远端口号   
    unsigned short dst_port; //目的端口号   
    unsigned short uhl;      //udp头部长度   
    unsigned short chk_sum;  //16位udp检验和   
}udp_hdr;  
//ICMP头部，总长度4字节   
typedef struct _icmp_hdr  
{  
    unsigned char icmp_type;   //类型   
    unsigned char code;        //代码   
    unsigned short chk_sum;    //16位检验和   
}icmp_hdr; 


#define TCPHDR_FIN 0x01  
#define TCPHDR_SYN 0x02  
#define TCPHDR_RST 0x04  
#define TCPHDR_PSH 0x08  
#define TCPHDR_ACK 0x10  
#define TCPHDR_URG ox20  
#define TCPHDR_ECE ox40  
#define TCPHDR_CWR ox80

TCP源码解读
http://blog.csdn.net/zhangskd/article/category/873810/2

URG         紧急指针(urgent pointer)有效
ACK          确认序号有效
PSH          指示接收方应该尽快将这个报文段交给应用层而不用等待缓冲区装满
RST           一般表示断开一个连接
SYN          同步序号用来发起一个连接
FIN            发送端完成发送任务(即断开连接)
ECN          显式拥塞通知,在TCP三次握手时表明TCP端是否支持ECN；在传输数据时表明接收到的
TCP段的IP首部的ECN被设置为11，即接收端发现了拥塞
CWR         为发送端缩小拥塞窗口标志，用来通知接收端它已经收到了设置ECE标志的ACK

kind=0是选项表结束选项
kind=1是空操作（nop）选项，没有特殊含义，一般用于将TCP选项的总长度填充为4字节的整数倍
kind=2是最大报文段长度选项
kind=3是窗口扩大因子选项,若接收通告窗口大小是N，窗口扩大因子（移位数）是M，那么TCP报文段的实际接收通告窗口大小是N左移M(0~14)位。
kind=4是选择性确认（Selective Acknowledgment，SACK）选项,它使TCP模块只重新发送丢失的TCP报文段，不用发送所有未被确认的TCP报文段
kind=5是SACK实际工作的选项
kind=8是时间戳选项。该选项提供了较为准确的计算通信双方之间的回路时间（Round Trip Time，RTT）的方法，从而为TCP流量控制提供重要信息

对于建链接的3次握手，主要是要初始化Sequence Number 的初始值。通信的双方要互相通知对方自己的初始化的Sequence Number（缩写为ISN：Inital Sequence Number）——所以叫SYN，全称Synchronize Sequence Numbers。也就上图中的 x 和 y。这个号要作为以后的数据通信的序号，以保证应用层接收到的数据不会因为网络上的传输的问题而乱序（TCP会用这个序号来拼接数据）

对于4次挥手，其实你仔细看是2次，因为TCP是全双工的，所以，发送方和接收方都需要Fin和Ack。只不过，有一方是被动的，所以看上去就成了所谓的4次挥手。如果两边同时断连接，那就会就进入到CLOSING状态，然后到达TIME_WAIT状态。下图是双方同时断连接的示意图（你同样可以对照着TCP状态机看）：

关于建连接时SYN超时。试想一下，如果server端接到了clien发的SYN后回了SYN-ACK后client掉线了，server端没有收到client回来的ACK，那么，这个连接处于一个中间状态，即没成功，也没失败。于是，server端如果在一定时间内没有收到的TCP会重发SYN-ACK。在Linux下，默认重试次数为5次，重试的间隔时间从1s开始每次都翻售，5次的重试时间间隔为1s, 2s, 4s, 8s, 16s，总共31s，第5次发出后还要等32s都知道第5次也超时了，所以，总共需要 1s + 2s + 4s+ 8s+ 16s + 32s = 2^6 -1 = 63s，TCP才会把断开这个连接。

SYN,Synchronize Sequence Numbers,通信的双方要互相通知对方自己的ISN,这个号要作为以后的数据通信的序号，以保证应用层接收到的数据不会因为网络上的传输的问题而乱序（TCP会用这个序号来拼接数据）
ISN,Inital Sequence Number,32位整数,非hard code,和一个假的时钟绑定,4微妙自增,4.55小时重复
MSL,Maximum Segment Lifetime,TCP Segment在网络上的最大存活时间

server端接到了clien发的SYN后回了SYN-ACK后client
掉线了,server端没有收到client回来的ACK，那么，这个
连接处于一个中间状态，即没成功，也没失败。于是，server端如果在一定时间内没有收到的TCP会重发SYN-ACK。在Linux下，默认重试次数为5次，重试的间隔时间从1s开始每次都翻售，5次的重试时间间隔为1s, 2s, 4s, 8s, 16s，总共31s，第5次发出后还要等32s都知道第5次也超时了，所以，总共需要 1s + 2s + 4s+ 8s+ 16s + 32s = 2^6 -1 = 63s，TCP才会把断开这个连接。

SYN Flood(DDos)攻击
症状:服务器的syn连接的队列耗尽，让正常的连接请求不能处理。
原因:服务端收不到客户端的SYN-ACK,导致重发,重试多次，时间消耗大
应对策略:tcp_syncookies,tcp_synack_retries,tcp_max_syn_backlog,tcp_abort_on_overflow

TIME_WAIT数量太多
端口被占用,内存消耗大,无可用句柄
连接关闭太频繁,无法及时释放资源,需要等待2MSL
tcp_tw_reuse,tcp_tw_recycle,tcp_max_tw_buckets,使用长连接KeepAlive

超时重传机制

最大特点是--忽略重传，不把重传的RT

最大特点是--忽略重传，不把重传的RTT做采样
严重问题--网络抖动(慢),RTT变大,导致不断重传
取巧方式--只要一发生重传,就对现有的RTO值翻倍（这就是所谓的 Exponential backoff）




计算平滑RTT
SRTT = SRTT + α (RTT – SRTT) 
计算平滑RTT和真实的差距（加权移动平均）
DevRTT = (1-β)*DevRTT + β*(|RTT-SRTT|)
神一样的公式
RTO= μ * SRTT + γ *DevRTT

nobody knows why, it just works…
在Linux下,α = 0.125,β = 0.25,μ = 1,γ = 4 
见
http://lxr.free-electrons.com/source/net/ipv4/tcp_input.c?v=2.6.32#L609

TCP必需要解决的可靠传输以及包乱序（reordering）的问题，所以，
TCP必需要知道网络实际的数据处理带宽或是数据处理
速度，这样才不会引起网络拥塞,导致丢包。
可靠传输
包乱序（reordering）

接收方
LastByteRead:TCP缓冲区中读到的位置
NextByteExpected:收到的连续包的最后一个位置
LastByteRcved:收到的包的最后一个位置

发送方
LastByteAcked:被接收端Ack过的位置（表示成功发送确认）
LastByteSent:已经发出去了,但还没有收到成功确认的Ack
LastByteWritten:上层应用正在写的地方

接收端在给发送端回ACK中会汇报自己的AdvertisedWindow = MaxRcvBuffer – LastByteRcvd – 1;
而发送方会根据这个窗口来控制发送数据的大小，以保证接收方可以处理


第一个图
分成了四个部分，分别是(其中那个黑模型就是滑动窗口)
#1已收到ack确认的数据。
#2发还没收到ack的。
#3在窗口中还没有发出的（接收方还有空间）。
#4窗口以外的数据（接收方没空间）
第二个图
是个滑动后的示意图（收到36的ack，并发出了46-51的字节）

零窗口探头ZWP(Zero Window Probe)
发送端
Window变成 0了,就不发数据了
发送 ZWP消息询问,连发三次,30~60s一次
RST关闭连接

接收方
Window 可用了,应答 ZWP消息,包含Window Size

ZWP DDoS 
攻击者和HTTP服务建好连接发完GET请求后,把Window设置为0,迫使服务端进行ZWP。攻击者会并发大量请求，把服务器端的资源耗尽

糊涂窗口综合症(Silly Window Syndrome)
接收方
太忙(CPU,内存),Receive Windows里数据多
Windows字节小,越来越小

发送方
Windows越来越小,每次传输越来越小,传几个字节

问题
IP,TCP 包含固定报文头,MTU每次需要接收最下数据,严重浪费带宽

解决
David DClark's方案
由Receiver端引起的。收到的数据导致window size小于某个值，可以直接ack(0)回sender，这样就把window给关闭了,也阻止了sender再发数据过来，等到receiver端处理了一些数据后windows size 大于等于了MSS，或者，receiver buffer有一半为空，就可以把window打开让send 发送数据过来。

Nagle's algorithm
由Sender端引起的。算法的思路也是延时处理,触发条件：1）要等到 Window Size>=MSS 或是 Data Size >=MSS，2）等待时间或是超时200ms，这两个条件有一个满足，他才会发数据，否则就是在攒数据。
见tcp_nagle_check函数
http://lxr.free-electrons.com/source/net/ipv4/tcp_output.c



Congestion Handling
Sliding Window来做流量控制还不够,只能解决收发两端,采样了RTT并计算RTO,都是为了更好的重传,
重传会导致网络的负担更重,于是会导致更大的延迟以及更多的丢包
TCP的还应该更聪明地知道整个网络上的事,知道网络中间发生了什么

慢启动
拥塞避免
拥塞发生
快速恢复

TCP不是一个自私的协议，当拥塞发生的时候，要做自我牺牲。就像交通阻塞一样，每个车都应该把路让出来，而不要再去抢路了。

刚刚加入网络的连接,需要一点一点地提速
慢启动的算法如下(cwnd全称Congestion Window)
连接建好的开始先初始化cwnd = 1，表明可以传一个MSS大小的数据
每当收到一个ACK,cwnd++,呈线性上升
每当过了一个RTT,cwnd = cwnd*2, 呈指数上升
还有一个ssthresh,是一个上限，当cwnd >= ssthresh时,进入 "拥塞避免算法"
ssthresh(slow start threshold),缓慢开始临界
cwnd:(Congestion Window),拥塞窗口

拥塞避免算法(Congestion Avoidance)
收到一个ACK时,cwnd = cwnd + 1/cwnd
当每过一个RTT时,cwnd = cwnd + 1

前面我们说过，，会有两种情况：
当丢包的时候
1）等到RTO超时,重传数据包。TCP认为这种情况太糟糕，反应也很强烈。
sshthresh =  cwnd /2
cwnd 重置为 1
进入慢启动过程

2）Fast Retransmit算法,
在收到3个duplicate ACK时就开启重传,而不用等到RTO超时
cwnd = cwnd / 2
sshthresh = cwnd
进入快速恢复算法——Fast Recovery

上面我们可以看到RTO超时后，sshthresh会变成cwnd的一半，这
意味着，如果cwnd<=sshthresh时出现的丢包，那么
TCP的sshthresh就会减了一半，然后等cwnd又很快地
以指数级增涨爬到这个地方时，就会成慢慢的线性增涨。
我们可以看到，TCP是怎么通过这种强烈地震荡快速而
小心得找到网站流量的平衡点的。


等到RTO超时，重传数据包。TCP认为这种情况太糟糕，反应也很强烈。
sshthresh =  cwnd /2,cwnd = 1
进入慢启动过程

快速恢复算法(Fast Recovery)(RFC5681)
拥塞发生sshthresh =  cwnd /2,cwnd = 1
cwnd = sshthresh  + MSS * 3(是确认有3个数据包被收到了)
重传Duplicated ACKs指定的数据包
如果再收到 duplicated Acks，那么cwnd = cwnd +1
如果收到了新的Ack,那么cwnd = sshthresh ,然后就开始拥塞避免算法

通过这种强烈地震荡快速而小心得找到网站流量的平衡点的

算法问题，它依赖于3个重复的Acks,超时一个窗口就减半一下，多个超时会超成TCP的传输速度呈级数下降，而且也不会触发Fast Recovery算法了

TCP New Reno(RFC 6582):Reno算法的优化版本,
HSTCP(High Speed TCP) 算法:
cwnd = cwnd + α(cwnd) / cwnd,cwnd = (1- β(cwnd))*cwnd
TCP WestWood算法:比较适用于无线网络,主要思想是通过在发送端不断的检测ack的到达速率来进行带宽估计
TCP BIC 算法:使用二分查找和检索的模式来确定合适的 Congestion Window

CUBIC在设计上简化了BIC-TCP的窗口调整算法，在BIC-TCP的窗口调整中会出现一个凹和凸(这里的凹和凸指的是数学意义上的凹和凸，凹函数/凸函数)的增长曲线，CUBIC使用了一个三次函数(即一个立方函数)，在三次函数曲线中同样存在一个凹和凸的部分，该曲线形状和BIC-TCP的曲线图十分相似，于是该部分取代BIC-TCP的增长曲线。另外，CUBIC中最关键的点在于它的窗口增长函数仅仅取决于连续的两次拥塞事件的时间间隔值，从而窗口增长完全独立于网络的时延RTT，之前讲述过的HSTCP存在严重的RTT不公平性，而CUBIC的RTT独立性质使得CUBIC能够在多条共享瓶颈链路的TCP连接之间保持良好的RTT公平性。


GET / HTTP/1.1
Host: www.careland.com.cn
Connection: keep-alive
Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 6.1; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/50.0.2661.102 Safari/537.36
Accept-Encoding: gzip, deflate, sdch
Accept-Language: zh-CN,zh;q=0.8


状态码
200 OK,请求已成功,响应头或数据体将随此响应返回
305 Use Proxy,被请求的资源必须通过指定的代理才能被访问
404 Not Found,请求失败,资源未被在服务器上发现
500 Internal Server Error,未曾预料的状况,无法完成请求的处理
502 Bad Gateway,作为网关或者代理工作的服务器接收到无效的响应




HTTP消息类型

请求(Request)消息：由客户端发给服务器的消息。
其组成包括:请求行(Request-Line)，可选的头域
(Header Field )，及实体(Entity-Body)

响应(Response)消息：是服务端回复客户端请求的消
息，其组成包括状态行(Status-Line)，可选的头域
(Header Field )，及实体(Entity-Body)

请求消息结构：
Full-Request = Request-Line
				*(General-Header
				    | Request-Header
				    | Entity-Header)
				CRLF
				[Entity-Body]
				
请求行结构
Request-Line =  Method SP 
			  Request-URI
			  SP 
			  HTTP-Version CRLF

eg: GET http://www.yesky.com/pub/WWW/page.html HTTP/1.1

请求消息示例：
GET http://www.yesky.com/pub/WWW/page.html HTTP/1.1
Connection：close
User-agent：Mozilla/4.0
Accept-Encoding：gzip,compress
Accept-language：en
CR LF
Entity-Body



首部(header),可以是一个或者多个首部,格式是 Name:SPValue


POST /navi/v1/fcgi/echo.fcgi HTTP/1.1\r\n
Host: six.dev1\r\n
Connection: keep-alive\r\n
Content-Length: 17\r\n
Cache-Control: max-age=0\r\n
Accept: text/html,application/xhtml+xml,...;q=0.8\r\n
User-Agent: Mozilla/5.0 ... Chrome/50.0.2661.102 Safari/537.36\r\n
Content-Type: application/x-www-form-urlencoded\r\n
Accept-Encoding: gzip, deflate\r\n
Accept-Language: zh-CN,zh;q=0.8\r\n
\r\n
fname=huangzhihui

编码后的内容
Content-Type: text/html
Content-Encoding: gzip
Content-Length: 19000


任何不含有消息体的消息(如1XX,204,304等响应消息和任何头(HEAD首部)请求的响应消息)总是由一个空行(CLRF)结束
如果出现了Transfer-Encoding头字段 并且值为非"identity",那么transfer-length由"chunked"传输编码定义,除非消息由于关闭连接而终止
如果出现了Content-Length头字段,它的值表示entity-length(实体长度)和transfer-length(传输长 度)。如果这两个长度的大小不一样(eg:设置了Transfer-Encoding头字段),那么将不能发送Content-Length头字段。并 且如果同时收到了Transfer-Encoding字段和Content-Length头字段,那么必须忽略Content-Length字段
如果消息使用媒体类型“multipart/byteranges”，并且transfer-length 没有另外指定，那么这种自定界（self-delimiting）媒体类型定义transfer-length 。除非发送者知道接收者能够解析该类型，否则不能使用该类型。
由服务器关闭连接确定消息长度。（注意：关闭连接不能用于确定请求消息的结束，因为服务器不能再发响应消息给客户端了。）


Content-Encoding
deflate
RFC1951说明的zlib格式,使用LZ77和哈弗曼进行编码
compress
采用unix的压缩格式
gzip
采用gun gzip的压缩编码,gzip = gzip头(10字节) + deflate编码的实际内容 + gzip尾(8字节校验,可选 crc32 和 adler32)
identity
表明实体没有进行编码

deflate(RFC1951):一种压缩算法，使用LZ77和哈弗曼进行编码；  
zlib(RFC1950):一种格式，是对deflate进行了简单的封装；  
gzip(RFC1952):一种格式，也是对deflate进行的封装.
可以看出deflate是最核心的算法，而zlib和gzip格式的区别仅仅是头部和尾部不一样，而实际的内容都是deflate编码的，即：
gzip = gzip头(10字节) + deflate编码的实际内容 + gzip尾(8字节)
[GZIP的实现可参考GzipOutputStream.java]
zlib = zlib头 + deflate编码的实际内容 + zlib尾
访问www.163.com. 响应报文含有gzip头，而www.baidu.com的响应报文没有gzip头。
看到gzip大家都很好的支持，有无gzip头都没有问题。

常见的媒体格式类型如下：

text/html:HTML格式   text/plain:纯文本格式   text/xml:XML格式
image/gif:gif图片格式  image/png:png图片格式 image/jpeg:jpg图片格式 


application/xhtml+xml:XHTML格式
application/xml:XML数据格式
application/json:JSON数据格式
application/msword:Word文档格式
application/octet-stream: 二进制流数据（如常见的文件下载）
application/x-www-form-urlencoded
<form encType="">中默认的encType，form表单数据被编码为key/value格式发送到服务器
application/x-protobuf:protobuf格式数据

multipart/form-data:多种类型混合数据

16进制值代表的数据长度CRLF
实际数据          
CRLF          //每段数据结束后，以\r\n标识

16进制代表的第二段数据\r\n
实际数据
CRLF        //每段数据结束后，以\r\n标识

………… (反复通过这样的方式表示每次传输的数据长度)

0CRLF     //数据结束部分用0表示,最后一块


HTTP/1.1 200 OK 
Content-Type: text/plain 
Transfer-Encoding: chunked

24
This is the data in the first chunk.

1B
and this is the second one.

and this is the second one.

3 
con 
8 
sequence 
0 


Remote Procedure Call(远程过程调用),调用远程计算机上的服务,就像调用本地服务一样

JSON-RPC是一个无状态且轻量级的远程过程调用(RPC)协议。它允许运行在基于socket,http等诸多不同消息传输环境的同一进程中。其使用Json(RFC4627)作为数据格式

请求数据格式
应答格式
{"result":"调用结果","error":"错误原因","id":调用标识}         
{ "method": "远程方法名称", "params": Json格式的参数, "id": 调用标识}

{ "method": "sayHello", "params": ["Hello JSON-RPC"], "id": 1}

{"result":"Hello JSON-RPC","error":null,"id":1}

https://program111.yanfa6.int/svn/docs/trunk/specification/案例分析/TCP&HTTP协议简介
"jsonrpc": "2.0",


The error codes from and including -32768 to -32000 are reserved for pre-defined errors.
code	message	meaning
-32700	Parse error	Invalid JSON was received by the server.
An error occurred on the server while parsing the JSON text.
-32600	Invalid Request	The JSON sent is not a valid Request object.
-32601	Method not found	The method does not exist / is not available.
-32602	Invalid params	Invalid method parameter(s).
-32603	Internal error	Internal JSON-RPC error.
-32000 to -32099	Server error	Reserved for implementation-defined server-errors.
 

code
message
A String providing a short description of the error.
The message SHOULD be limited to a concise single sentence.
data
A Primitive or Structured value that contains additional information about the error.
This may be omitted.
The value of this member is defined by the Server (e.g. detailed error information, nested errors etc.).

反向代理和负载均衡