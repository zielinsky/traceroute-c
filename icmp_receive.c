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

void print_as_bytes (unsigned char* buff, ssize_t length)
{
    for (ssize_t i = 0; i < length; i++, buff++)
        printf ("%.2x ", *buff);
}

double *recv_from(int sock_fd, const char* from_ip, int id, int tries){
    double *res = malloc(tries * sizeof(double));
    bzero(res, tries * sizeof(double));

    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    u_int8_t buffer[IP_MAXPACKET];

    struct pollfd ps;
    ps.fd = sock_fd;
    ps.events = POLLIN;
    ps.revents = 0;
    int ready;

    double t = get_time();
    while ((get_time() - t) <= 1.0 && tries > 0) {
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
                res[ip_seq] = get_time();
                tries--;
            }

        }
    }
    return res;
}
