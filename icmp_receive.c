#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>

double get_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec*1e-9;
}

int recv_from(int sock_fd, const char* from_ip, int id, int tries, const double *send_time){
    int reply_number = 0;
    int ret = 1;
    double *elapsed_time = malloc(tries * sizeof(double));
    char ips[tries][20];
    bzero(elapsed_time, tries * sizeof(double));

    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    struct pollfd ps;
    ps.fd = sock_fd;
    ps.events = POLLIN;
    ps.revents = 0;
    int ready;

    double t = get_time();
    while ((get_time() - t) <= 1.0 && reply_number < tries) {
        ready = poll (&ps, 1, 1000);

        while(ready-- && ps.revents == POLLIN){
            ssize_t packet_len = recvfrom (sock_fd, buffer, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr*)&sender, &sender_len);
            if (packet_len < 0) {
                fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
                continue;
            }

            char sender_ip_str[20];
            inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));

            struct ip *ip = (struct ip *)buffer;
            ssize_t	ip_header_len = 4 * (ssize_t)(ip->ip_hl);

            struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header_len);

            // TTL time out
            char original_dest_ip_str[20];
            if (icmp_header->type == 11) {
                icmp_header = (struct icmphdr *) (buffer + ip_header_len + 28);
                ip = (struct ip *) (buffer + ip_header_len + 8);

                struct in_addr dst = ip->ip_dst;
                inet_ntop(AF_INET, &dst, original_dest_ip_str, sizeof(original_dest_ip_str));
            }else{
                strcpy(original_dest_ip_str, sender_ip_str);
            }

            int ip_id = ntohs(icmp_header->un.echo.id);
            int ip_seq = ntohs(icmp_header->un.echo.sequence);

            if(ip_id == id && (strcmp(original_dest_ip_str, from_ip) == 0)){
                strcpy(ips[ip_seq], sender_ip_str);
                if(strcmp(sender_ip_str, from_ip) == 0){
                    ret = 0;
                }
                elapsed_time[ip_seq] = get_time() - send_time[ip_seq];
                reply_number++;
            }

        }
    }
    for(int trs = 0; trs < tries; trs++){
        if((int)(elapsed_time[trs]*1000) == 0)
            continue;
        int guardian = 0;
        for(int i = 0; i < trs; i++){
            if(strcmp(ips[trs], ips[i]) == 0){
                guardian = -1;
            }
        }
        if(guardian == 0){
            printf("%s ", ips[trs]);
        }
    }

    for(int trs = 0; trs < tries; trs++){
        int elapsed_times = (int)(elapsed_time[trs]*1000);
        if(elapsed_times == 0){
            printf("* ");
        }else{
            printf("%dms ", elapsed_times);
        }
    }
    printf("\n");
    return ret;
}
