/*
Do not modify this file
*/
#include "common.h"
#include <random>

int main()
{

    // setup receive socket
    int sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_UDP);
    if (sock_fd < 0)
    {
        perror("server socket creation failed");
        exit(-1);
    }

    int enable = 1;
    if (setsockopt(sock_fd, IPPROTO_IP, IP_HDRINCL, &enable, sizeof(enable)) < 0)
    {
        perror("setsockopt failed");
        exit(-1);
    }

    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = inet_addr(SERVER_IP);
    server.sin_port = htons(SERVER_PORT);

    if (bind(sock_fd, (sockaddr *)&server, sizeof(server)) < 0)
    {
        perror("bind failed");
        exit(-1);
    }

    std::mt19937 g;
    std::uniform_int_distribution<int> d(0, NKEYS - 1);

    char buffer[PACKET_LEN];
    ipheader *ip = (ipheader *)buffer;
    udpheader *udp = (udpheader *)(buffer + sizeof(ipheader));
    char *data = buffer + sizeof(ipheader) + sizeof(udpheader);
    while (true)
    {
        printf("Listening ... \n");
        // int bytes = recv(sock_fd, buffer, PACKET_LEN, 0);
        sockaddr_in info;
        socklen_t info_size = sizeof(info);
        int bytes = recvfrom(sock_fd, buffer, PACKET_LEN, 0, (sockaddr *)&info, &info_size);
        if (bytes < 0)
        {
            perror("recvfrom failed");
            exit(-1);
        }
        buffer[bytes] = 0;
        // grab source ip
        char *src_ip = inet_ntoa(ip->iph_sourceip);
        printf("Source ip: %s, Data: %s (packet: %d, data: %ld bytes)\n", src_ip, data, bytes, bytes - sizeof(ipheader) - sizeof(udpheader));
        // if this is the spoofed ip; send back response key
        if (strcmp(src_ip, SPOOF_IP) == 0)
        {
            // add response data
            int k_idx = d(g);
            printf("%d\n", k_idx);
            const char *echo = KEYS[k_idx];
            int echo_len = strlen(echo);

            // create ip header
            ip->iph_ver = 4;
            ip->iph_ihl = 5;
            ip->iph_ttl = 20;
            ip->iph_sourceip.s_addr = inet_addr(SERVER_IP);
            ip->iph_destip.s_addr = inet_addr(data);
            ip->iph_protocol = IPPROTO_UDP;
            ip->iph_len = htons(sizeof(struct ipheader) +
                                sizeof(struct udpheader) + echo_len);
            ip->iph_chksum = 0;

            // create udp header
            udp->udp_dport = htons(CLIENT_PORT);
            udp->udp_sport = htons(SERVER_PORT);
            udp->udp_ulen = htons(sizeof(udpheader) + echo_len);
            udp->udp_sum = 0;

            // overwrite incoming data with response data
            strncpy(data, echo, echo_len);

            // send multiple times reduce packet loss probability
            for (int i = 0; i < MAX_ATTEMPT; ++i)
            {
                sleep(1);
                printf("Attempt: %d/%d ...\n", i + 1, MAX_ATTEMPT);
                send_raw_ip_packet(ip);
            }
        }
        // else: discard packet
    }

    close(sock_fd);

    return 0;
}