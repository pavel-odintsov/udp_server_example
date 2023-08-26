#include <array>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

#include <netdb.h>
#include <string.h>
#include <sys/socket.h>

uint64_t processed_udp_packets      = 0;
uint64_t processed_udp_packets_prev = 0;

bool create_and_bind_socket(const std::string& host, unsigned int port, int& sockfd) {
    std::cout << "Netflow plugin will listen on " << host << ":" << port << " udp port" << std::endl;

    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);

    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    // AI_PASSIVE to handle empty host as bind on all interfaces
    // AI_NUMERICHOST to allow only numerical host
    hints.ai_flags = AI_PASSIVE | AI_NUMERICHOST;

    addrinfo* servinfo = NULL;

    std::string port_as_string = std::to_string(port);

    int getaddrinfo_result = getaddrinfo(host.c_str(), port_as_string.c_str(), &hints, &servinfo);

    if (getaddrinfo_result != 0) {
        std::cout << "getaddrinfo function failed with code: " << getaddrinfo_result << " please check host syntax" << std::endl;
        return false;
    }

    sockfd = socket(servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);

    int bind_result = bind(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);

    if (bind_result) {
        std::cout << "Can't bind on port: " << port << " on host " << host << " errno:" << errno
                  << " error: " << strerror(errno) << std::endl;

        return false;
        ;
    }

    std::cout << "Successful bind" << std::endl;

    // Free up memory for server information structure
    freeaddrinfo(servinfo);

    return true;
}

void capture_traffic_from_socket(int sockfd) {
    std::cout << "Started capture" << std::endl;

    while (true) {
        const unsigned int udp_buffer_size = 65536;
        char udp_buffer[udp_buffer_size];

        int received_bytes = recv(sockfd, udp_buffer, udp_buffer_size, 0);

        if (received_bytes > 0) {
            processed_udp_packets++;
        }
    }
}

void print_speed() {
    processed_udp_packets_prev = processed_udp_packets;

    std::cout << "UDP packets / second" << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        std::cout << processed_udp_packets - processed_udp_packets_prev << std::endl;

        processed_udp_packets_prev = processed_udp_packets;
    }
}

int main() {
    std::string host = "::";
    uint32_t port    = 2055;

    int socket_fd = 0;

    bool result = create_and_bind_socket(host, port, socket_fd);

    if (!result) {
        std::cout << "Cannot create / bind socket" << std::endl;
        exit(1);
    }


    std::cout << "Starting packet capture" << std::endl;

    std::thread current_thread(capture_traffic_from_socket, socket_fd);

    // Start speed printer
    std::thread speed_printer(print_speed);

    // Wait until both threads finish
    current_thread.join();
    speed_printer.join();
}
