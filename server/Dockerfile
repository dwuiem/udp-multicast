FROM gcc:latest
WORKDIR /app
COPY server.cpp .
RUN g++ -std=c++17 -o server server.cpp
CMD ["./server"]