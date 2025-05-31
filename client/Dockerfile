FROM gcc:latest
WORKDIR /app
COPY client.cpp .
RUN g++ -std=c++17 -o client client.cpp
RUN apt-get update && apt-get install -y iptables iputils-ping
CMD ["sh", "-c", "SERVER_IP=$(ping -c1 $SERVER | grep PING | awk '{print $3}' | tr -d '()') && iptables -A INPUT -s $SERVER_IP -p udp --dport 12345 -j ACCEPT && iptables -A INPUT -j DROP && ./client"]