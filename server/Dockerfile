FROM gcc:latest

WORKDIR /app

COPY server.cpp .
COPY entrypoint.sh .

RUN g++ -std=c++17 -o server server.cpp \
 && chmod +x entrypoint.sh \
 && apt-get update \
 && apt-get install -y iptables iputils-ping

CMD ["./entrypoint.sh"]
