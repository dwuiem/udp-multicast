#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <sys/utsname.h>

// Constants for network configuration
const char* MULTICAST_ADDR = "239.255.1.1";
constexpr int PORT = 12345;
constexpr int MAX_BUFFER = 1024;

// Client class to handle multicast reception and responses
class MonitoringClient {
private:
    int sockfd{};
    sockaddr_in server_addr{}, client_addr{};
    std::string client_id;

    // Initialize UDP socket and join multicast group
    void init_socket() {
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            throw std::runtime_error("Failed to create socket");
        }

        // Allow multiple sockets to use the same port
        constexpr int reuse = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        // Bind to receive multicast messages
        memset(&client_addr, 0, sizeof(client_addr));
        client_addr.sin_family = AF_INET;
        client_addr.sin_addr.s_addr = INADDR_ANY;
        client_addr.sin_port = htons(PORT);

        if (bind(sockfd, reinterpret_cast<sockaddr *>(&client_addr), sizeof(client_addr)) < 0) {
            throw std::runtime_error("Failed to bind socket");
        }

        // Join multicast group
        ip_mreq mreq{};
        mreq.imr_multiaddr.s_addr = inet_addr(MULTICAST_ADDR);
        mreq.imr_interface.s_addr = INADDR_ANY;
        if (setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq)) < 0) {
            throw std::runtime_error("Failed to join multicast group");
        }

        // Set up server address for responses
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY; // Will be updated dynamically
        server_addr.sin_port = htons(PORT);
    }

    // Get system information
    static std::string get_info() {
        utsname info{};
        uname(&info);
        return std::string("Hostname: ") + info.nodename;
    }

    // Process incoming messages
    void process_message(const std::string& message, const sockaddr_in& sender) {
        if (message.find("DISCOVER:") == 0) {
            std::string response = "RESPONSE:" + message.substr(9);
            server_addr.sin_addr = sender.sin_addr; // Update server address
            sendto(sockfd, response.c_str(), response.length(), 0,
                   reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
            std::cout << "Received DISCOVER, sent RESPONSE to " << inet_ntoa(sender.sin_addr) << std::endl;
        } else if (message == "SYSINFO_REQUEST") {
            std::string sysinfo = "SYSINFO:" + get_info();
            sendto(sockfd, sysinfo.c_str(), sysinfo.length(), 0,
                   reinterpret_cast<sockaddr *>(&server_addr), sizeof(server_addr));
            std::cout << "Sent system info to " << inet_ntoa(server_addr.sin_addr) << std::endl;
        }
    }

public:
    MonitoringClient() {
        init_socket();
        client_id = std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }

    ~MonitoringClient() {
        close(sockfd);
    }

    // Main client loop
    void run() {
        char buffer[MAX_BUFFER];
        sockaddr_in sender_addr{};
        socklen_t addr_len = sizeof(sender_addr);

        while (true) {
            const ssize_t bytes = recvfrom(sockfd, buffer, MAX_BUFFER - 1, 0,
                                 reinterpret_cast<struct sockaddr *>(&sender_addr), &addr_len);
            if (bytes < 0) {
                std::cerr << "Error receiving message" << std::endl;
                continue;
            }

            buffer[bytes] = '\0';
            process_message(std::string(buffer), sender_addr);
        }
    }
};

int main() {
    try {
        MonitoringClient client;
        client.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}