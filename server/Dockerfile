FROM gcc:latest
WORKDIR /app
COPY server.cpp .
RUN g++ -std=c++17 -o server server.cpp
RUN apt-get update && apt-get install -y iptables iputils-ping
CMD ["sh", "-c", "for client in $(echo $CLIENTS | tr ',' ' '); do CLIENT_IP=$(ping -c1 $client | grep PING | awk '{print $3}' | tr -d '()') && iptables -A INPUT -s $CLIENT_IP -p udp --dport 12345 -j ACCEPT; done && iptables -A INPUT -j DROP && ./server"]