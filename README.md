# Udp_Owamp
一款轻量级的网络性能测量工具。

# Obtaining
git clone https://github.com/LiuNotTwo/udp_owamp.git

# Building
cd udp_owamp; make

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
![image](https://github.com/LiuNotTwo/udp_owamp/blob/main/example/figs/output_example.PNG)
