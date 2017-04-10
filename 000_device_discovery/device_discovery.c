#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define DEST_PORT 20055
#define BUF_SIZE 1 << 8

int main(int argc, char **argv)
{

  int sockfd;
  int broadcast = 1;
  struct sockaddr_in addr, faddr;
  socklen_t faddr_len;

  int bytes_sent;
  char buf[BUF_SIZE];
  int buflen = 0;

  struct timeval tv, select_tv;
  int maxfd;
  int select_rv;
  fd_set readfds;

  int bytes_recv;

  tv.tv_sec = 2;
  tv.tv_usec = 0;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(DEST_PORT);
  addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);

/*
  if (inet_pton(AF_INET, argv[1], &(addr.sin_addr)) != 1)
  {
    fprintf(stderr, "%s: Could not parse IPv4 address: '%s'!\n", argv[0], argv[1]);
    exit(1);
  }
*/

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
  {
    fprintf(stderr, "%s: Could not create socket!\n", argv[0]);
    exit(1);
  }

  if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0)
  {
    fprintf(stderr, "%s: Could not grant broadcast privileges to socket!\n", argv[0]);
    goto fail;
  }

  bytes_sent = sendto(sockfd, buf, buflen, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (bytes_sent < 0)
  {
    fprintf(stderr, "%s: Could not send buffer!\n", argv[0]);
    goto fail;
  }

/*
  printf("Sent %i bytes to %s.\n", bytes_sent, inet_ntoa(addr.sin_addr));
*/
  printf("Sent device discovery.\n");

  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  maxfd = sockfd;

  select_tv = tv;
  bytes_recv = 0;
  while (!bytes_recv)
  {
    errno = 0;
    select_rv = select(maxfd + 1, &readfds, NULL, NULL, &select_tv);
    if (select_rv > 0) {
      if (FD_ISSET(sockfd, &readfds)) {
        printf("Device responded!\n");
        bytes_recv = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&faddr, &faddr_len);
        if (bytes_recv > 0)
        {
          printf("Received %i bytes from %s.\n", bytes_recv, inet_ntoa(faddr.sin_addr));
        }
      }
    }
    else if (select_rv == 0)
    {
      printf("Device hasn't responded in %li seconds.\n", tv.tv_sec);
      break;
    }
    else
    {
      fprintf(stderr, "Select returned error '%s'.\n", strerror(errno));
      goto fail;
    }
  }

  close(sockfd);
  return 0;

fail:

  close(sockfd);
  return 1;

}
