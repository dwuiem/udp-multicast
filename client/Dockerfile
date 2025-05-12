FROM gcc:latest
WORKDIR /app
COPY client.cpp .
RUN g++ -std=c++17 -o client client.cpp
CMD ["./client"]