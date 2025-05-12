#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <thread>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>

// Constants for network configuration
const char* MULTICAST_ADDR = "239.255.1.1";
constexpr int PORT = 12345;
constexpr int TIMEOUT_SECONDS = 10;
constexpr int MAX_BUFFER = 1024;

// Structure to hold client information
struct ClientInfo {
    std::string ip;
    std::chrono::system_clock::time_point last_seen;
    std::string system_info;
    bool is_alive;
};

// Server class to manage multicast and client tracking
class MonitoringServer {
private:
    int sockfd{};
    sockaddr_in multicast_addr{}, server_addr{}, client_addr{};
    std::map<std::string, ClientInfo> clients; // Map of client IP to info
    std::chrono::system_clock::time_point last_discover;

    // Initialize UDP socket for multicast
    void init_socket() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Allow multiple sockets to use the same port
        int reuse = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Bind to receive responses
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(PORT);

        if (bind(sockfd, reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        // Set up multicast address
        memset(&multicast_addr, 0, sizeof(multicast_addr));
        multicast_addr.sin_family = AF_INET;
        multicast_addr.sin_addr.s_addr = inet_addr(MULTICAST_ADDR);
        multicast_addr.sin_port = htons(PORT);
    }

    // Send multicast discover message
    void send_discover() {
        const auto now = std::chrono::system_clock::now();
        const auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        std::string message = "DISCOVER:" + std::to_string(timestamp);

        if (sendto(sockfd, message.c_str(), message.length(), 0,
                   reinterpret_cast<sockaddr *>(&multicast_addr), sizeof(multicast_addr)) < 0) {
            std::cerr << "Failed to send multicast message" << std::endl;
        } else {
            last_discover = now;
            std::cout << "Sent multicast DISCOVER at " << format_time(now) << std::endl;
        }
    }

    // Receive client responses
    void receive_responses() {
        char buffer[MAX_BUFFER];
        socklen_t addr_len = sizeof(client_addr);

        // Set socket timeout
        timeval tv{};
        tv.tv_sec = TIMEOUT_SECONDS / 2;
        tv.tv_usec = 0;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        while (true) {
            const ssize_t bytes = recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0,
                                 reinterpret_cast<struct sockaddr *>(&client_addr), &addr_len);
            if (bytes < 0) break; // Timeout or error

            buffer[bytes] = '\0';
            std::string message(buffer);
            std::string client_ip = inet_ntoa(client_addr.sin_addr);

            if (message.find("RESPONSE:") == 0) {
                update_client(client_ip, message.substr(9));
            } else if (message.find("SYSINFO:") == 0) {
                clients[client_ip].system_info = message.substr(8);
            }
        }
    }

    // Update client status
    void update_client(const std::string& ip, const std::string& response) {
        auto now = std::chrono::system_clock::now();
        if (clients.find(ip) == clients.end()) {
            clients[ip] = {ip, now, "", true};
            std::cout << "New client detected: " << ip << std::endl;
        } else {
            clients[ip].last_seen = now;
            clients[ip].is_alive = true;
        }
    }

    // Check for dead clients
    void check_dead_clients() {
        auto now = std::chrono::system_clock::now();
        for (auto& [ip, client] : clients) {
            auto seconds_since_last_seen = std::chrono::duration_cast<std::chrono::seconds>(
                now - client.last_seen).count();
            if (seconds_since_last_seen > TIMEOUT_SECONDS && client.is_alive) {
                client.is_alive = false;
                std::cout << "Client " << ip << " marked as dead at " << format_time(now) << std::endl;
            }
        }
    }

    // Request system info from alive clients
    void request_info() {
        for (const auto& [ip, client] : clients) {
            if (client.is_alive) {
                sockaddr_in addr{};
                addr.sin_family = AF_INET;
                addr.sin_port = htons(PORT);
                inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

                std::string message = "SYSINFO_REQUEST";
                sendto(sockfd, message.c_str(), message.length(), 0,
                       reinterpret_cast<sockaddr *>(&addr), sizeof(addr));
            }
        }
    }

    // Format time for display
    static std::string format_time(const std::chrono::system_clock::time_point& tp) {
        auto time = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    // Pretty print client status
    void print_status() {
        std::cout << "\n=== Client Status ===" << std::endl;
        std::cout << std::left
                  << std::setw(15) << "IP Address"
                  << std::setw(20) << "Last Seen"
                  << std::setw(10) << "Status"
                  << std::setw(30) << "System Info" << std::endl;
        std::cout << std::string(75, '-') << std::endl;

        for (const auto& [ip, client] : clients) {
            std::cout << std::setw(15) << ip
                      << std::setw(20) << format_time(client.last_seen)
                      << std::setw(10) << (client.is_alive ? "Alive" : "Dead")
                      << std::setw(30) << client.system_info << std::endl;
        }
        std::cout << std::endl;
    }

public:
    MonitoringServer() {
        init_socket();
    }

    ~MonitoringServer() {
        close(sockfd);
    }

    // Main server loop
    void run() {
        while (true) {
            send_discover();
            receive_responses();
            check_dead_clients();
            request_info();
            print_status();
            std::this_thread::sleep_for(std::chrono::seconds(TIMEOUT_SECONDS));
        }
    }
};

int main() {
    try {
        MonitoringServer server;
        server.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}