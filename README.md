# Udp_Owamp
一款轻量级的网络性能测量工具。

# Obtaining
cent-os$ git clone https://github.com/LiuNotTwo/udp_owamp.git

# Building
cent-os$ cd udp_owamp

cent-os$ make

# Using
### server
./udp_owamp_server 0.0.0.0 19999 或者 ./udp_owamp_server 19999

注意，客户端与服务器端通过udp通信，如果服务器端防火墙开启，udp包可能会被过滤。

解决方法1. 关闭服务器端防火墙；

解决方法2. 开放相应端口：


&emsp;cent-os$ firewall-cmd --permanent --zone=public --add-port=19999/udp


&emsp;cent-os$ firewall-cmd --reload
    
### client

./udp_owamp_client server_addr 19999 或者 ./udp_owamp_server server_addr 19999 -c 10 -i 10

-c:指定发包数量，默认为10

-i:指定发包间隔，默认为100ms


# Output Example

SSN       RSN       Delay1    Delay2    RTT

1         1         0.226     0.465     0.691     

2         2         0.234     0.303     0.537     

3         3         0.210     0.293     0.503     

4         4         0.237     0.304     0.541     

5         5         0.241     0.302     0.543     

6         6         0.208     0.299     0.507     

7         7         0.218     0.305     0.523     

8         8         0.137     0.281     0.418     

9         9         0.304     0.354     0.658     

10        10        0.472     0.344     0.816     

===============================================

Two way delay:


&emsp;min/aver/max = 0.418/0.574/0.816 ms


One way delay:


&emsp;Source->Dest: min/aver/max = 0.137/0.249/0.472 ms


&emsp;Dest->Source: min/aver/max = 0.281/0.325/0.465 ms

===============================================

Two way jitter:


&emsp;0.087 ms

One way jitter:
    

&emsp;Source->Dest: 0.058 ms
    

&emsp;Dest->Source: 0.033 ms

===============================================

Two way packet loss rate:
    

&emsp;0.00%

One way packet loss rate:


&emsp;Source->Dest: 0.00%
    

&emsp;Dest->Source: 0.00%
