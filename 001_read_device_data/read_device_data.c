#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

#define DEST_PORT 20055
#define BUF_SIZE (1 << 6)

void memdump(const char *s, const int len)
{
  int i, j = len, k, l = 0;
  char c;

  while (j) {
    j /= 16;
    l++;
  }
  
  printf("Displaying: %d bytes\n\n", len);
  
  printf("%*s  ", l, "");
  for (i = 0; i < 16;) {
    printf("%2X", i++);
   if (i % 4 == 0) 
     printf(" ");
  }
  for (i = 0; i < len;) {
    printf("\n%0*X: ", l, i);
    for (j = 0; j < 16 && i < len; j++) {
      printf("%02hhx", s[i++]);
      if (i % 4 == 0) 
        printf(" ");
    }
    i -= j;
    for (k = 0; k < j; k++) {
      c = s[i++];
      if (k % 8 == 0)
        printf(" ");
      if (c < 32 || c > 126) c = '.';  
        printf("%c", c);
    }
  }
  printf("\n");
}

char calculate_checksum(char *s)
{
  int i, sum = 0;

  for (i = 0; i < 7; i++)
    sum += *(s + i);

  return (char)(sum & 0xff);
}

int main(int argc, char **argv)
{
  int sockfd;
  struct sockaddr_in addr, faddr;
  socklen_t faddr_len;
  char str[INET_ADDRSTRLEN];

  int bytes_sent;
  int bytes_recv;

  char buf[BUF_SIZE];

  struct timeval tv, select_tv;
  int maxfd;
  int select_rv;
  fd_set readfds;

  tv.tv_sec = 2;
  tv.tv_usec = 0;

  if (argc < 2) {
    fprintf(stderr,"%s IP\n", argv[0]);
    exit(1);
  }

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(DEST_PORT);

  if (inet_pton(AF_INET, argv[1], &(addr.sin_addr)) != 1) {
    fprintf(stderr, "%s: Could not parse IPv4 address: '%s'!\n", argv[0], argv[1]);
    exit(1);
  }

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    fprintf(stderr, "%s: Could not create socket!\n", argv[0]);
    exit(1);
  }

  memset(&buf, 0, sizeof(buf));

  /* Prepare packet. */
  *buf = 0xbb;

  *(buf + 7) = calculate_checksum(buf);

  bytes_sent = sendto(sockfd, buf, 64, 0, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (bytes_sent < 0) {
    fprintf(stderr, "%s: Could not send buffer!\n", argv[0]);
    goto fail;
  }
  inet_ntop(AF_INET, &(addr.sin_addr), str, INET_ADDRSTRLEN);
  printf("Sent %i bytes to %s.\n", bytes_sent, str);

  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);

  maxfd = sockfd;

  select_tv = tv;
  bytes_recv = 0;
  while (!bytes_recv) {
    errno = 0;
    select_rv = select(maxfd + 1, &readfds, NULL, NULL, &select_tv);
    if (select_rv > 0) {
      if (FD_ISSET(sockfd, &readfds)) {
        faddr_len = sizeof(faddr);
        bytes_recv = recvfrom(sockfd, buf, sizeof(buf), 0, (struct sockaddr *)&faddr, &faddr_len);
        if (bytes_recv > 0)
        {
          inet_ntop(AF_INET, &(faddr.sin_addr), str, INET_ADDRSTRLEN);
          printf("Received %i bytes from %s.\n", bytes_recv, str);
          memdump(buf, bytes_recv);
          printf("\n");
        }
      }
    }
    else if (select_rv == 0) {
      printf("Device hasn't responded in %li seconds.\n", tv.tv_sec);
      break;
    }
    else {
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
