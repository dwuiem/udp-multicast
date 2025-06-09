FROM gcc:latest

WORKDIR /app

COPY client.cpp .
COPY entrypoint.sh .

RUN g++ -std=c++17 -o client client.cpp \
 && chmod +x entrypoint.sh \
 && apt-get update \
 && apt-get install -y iptables iputils-ping

CMD ["./entrypoint.sh"]
