// Patryk Zieliński 330261
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip_icmp.h>
#include <strings.h>
#include <stdio.h>
#include <assert.h>
#include "utils.h"

u_int16_t compute_icmp_checksum (const void *buff, int length)
{
    const u_int16_t* ptr = buff;
    u_int32_t sum = 0;
    assert (length % 2 == 0);
    for (; length > 0; length -= 2)
        sum += *ptr++;
    sum = (sum >> 16U) + (sum & 0xffffU);
    return (u_int16_t)(~(sum + (sum >> 16U)));
}

int set_ttl(int sock_fd, int ttl){
    if (setsockopt(sock_fd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) != 0) {
        fprintf(stderr, "socket error: problem with setting ttl(%d)!\n", ttl);
        return -1;
    }
    return 0;
}


int send_echo_request(int sock_fd, struct sockaddr_in *dest_addr, uint16_t id, uint16_t seq, double *send_time){
	struct icmphdr header;
    bzero(&header, sizeof(header));

    header.type = ICMP_ECHO;
    header.code = 0;
    header.checksum = 0;
    header.un.echo.id = htons(id);
    header.un.echo.sequence = htons(seq);

    header.checksum = compute_icmp_checksum((uint16_t *)&header, sizeof(header));;


    send_time[seq] = get_time();
    ssize_t bytes_send = sendto(sock_fd,&header, sizeof(header),0,(struct sockaddr *)dest_addr, sizeof(*dest_addr));
    if (bytes_send != sizeof(header)) {
        fprintf(stderr, "send more bytes(%zd) than header size(%zd)\n", bytes_send, sizeof(header));
        return -1;
    }

    return 0;
}

int send_n_echo_requests(int n, int ttl, int sock_fd, struct sockaddr_in *dest_addr, uint16_t id, double *send_time){
    if(set_ttl(sock_fd, ttl) != 0) return -1;
    for(int i = 0; i < n; i++) if(send_echo_request(sock_fd, dest_addr, id, i, send_time) != 0) return -1;
    return 0;
}
