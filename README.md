# PerfTrace
A lightweight network performance measurement tool.

# Obtaining
`git clone git@github.com:LiuNotTwo/PerfTrace.git`

# Building
```
cd PerfTrace
make
```

# Using
### server
`./perftrace_srv`

! Note that the client communicates with the server via UDP. If the server-side firewall is on, UDP packets may be filtered.

Solution 1: Disable the server-side firewall;

Solution 2: Open the specified UDP port (19999).

&emsp;cent-os$ firewall-cmd --permanent --zone=public --add-port=19999/udp

&emsp;cent-os$ firewall-cmd --reload
    
### client

##### mode1 (Basic Mode)

`./perftrace_cli -s server_ip -c 10 -i 10ms -m 1`

-c: Specify the number of packets to be sent, default is 10;

-i: Specify the packet sending interval, the default is 1ms; 

-m: Specify the measurement mode, the default is 1 (Basic Mode).

##### mode2 (AB Mode)

`./perftrace_cli -s server_ip -m 2`


# Output Example
##### mode1 (Basic Mode)
![image](https://github.com/LiuNotTwo/PerfTrace/blob/main/example/figs/basic_mode.PNG)

##### mode2 (AB Mode)
![image](https://github.com/LiuNotTwo/PerfTrace/blob/main/example/figs/AB_mode.PNG)
