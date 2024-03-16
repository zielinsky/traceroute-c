#include <arpa/inet.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <stdio.h>
#include <string.h>
#include <sys/poll.h>
#include <time.h>
#include <stdbool.h>

double get_time()
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    return (double)t.tv_sec + (double)t.tv_nsec*1e-9;
}

typedef struct replyInfo {
    double rtt;
    char address[20];
    bool received;
} replyInfo_t;

void print_results(replyInfo_t* replies, int request_count){
    bool is_unique;
    for(int i = 0; i < request_count; i++){
        if(!replies[i].received)
            continue;

        is_unique = true;
        for(int j = 0; j < i; j++){
            if(strcmp(replies[i].address, replies[j].address) == 0){
                is_unique = false;
                break;
            }
        }
        if(is_unique)
            printf("%s ", replies[i].address);

    }

    for(int i = 0; i < request_count; i++){
        if (replies[i].received)
            printf("%.3fms ", replies[i].rtt);
        else
            printf("* ");

    }
    printf("\n");
}

int recv_from(int sock_fd, const char*dest_address_str, int id, int request_count, const double *request_send_time){
    int reply_number = 0, return_value = 1;

    replyInfo_t replies[request_count];
    bzero(replies, request_count * sizeof(replyInfo_t));

    char sender_ip_str[20];
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    char original_dest_ip_str[20];

    struct pollfd ps;
    ps.fd = sock_fd;
    ps.events = POLLIN;
    ps.revents = 0;

    int ready;
    double start_time = get_time();
    while ((get_time() - start_time) <= 1.0 && reply_number < request_count) {
        ready = poll (&ps, 1, 1000);

        while(ready-- && ps.revents == POLLIN){
            ssize_t packet_len = recvfrom (sock_fd, buffer, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr*)&sender, &sender_len);
            if (packet_len < 0) {
                fprintf(stderr, "recvfrom error: %s\n", strerror(errno));
                continue;
            }

            inet_ntop(AF_INET, &(sender.sin_addr), sender_ip_str, sizeof(sender_ip_str));

            struct ip *ip = (struct ip *)buffer;
            ssize_t	ip_header_len = 4 * (ssize_t)(ip->ip_hl);

            struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header_len);

            // TTL time out
            if (icmp_header->type == 11) {
                icmp_header = (struct icmphdr *) (buffer + ip_header_len + 28);
                ip = (struct ip *) (buffer + ip_header_len + 8);

                struct in_addr dst = ip->ip_dst;
                inet_ntop(AF_INET, &dst, original_dest_ip_str, sizeof(original_dest_ip_str));
            }else
                strcpy(original_dest_ip_str, sender_ip_str);


            int ip_id = ntohs(icmp_header->un.echo.id);
            int ip_seq = ntohs(icmp_header->un.echo.sequence);

            if(ip_id == id && (strcmp(original_dest_ip_str, dest_address_str) == 0)){
                if(strcmp(sender_ip_str, dest_address_str) == 0){
                    return_value = 0;
                }
                strcpy(replies[ip_seq].address, sender_ip_str);
                replies[ip_seq].rtt = (get_time() - request_send_time[ip_seq]) * 1000;
                replies[ip_seq].received = true;
                reply_number++;
            }
        }
    }

    print_results(replies, request_count);
    return return_value;
}
