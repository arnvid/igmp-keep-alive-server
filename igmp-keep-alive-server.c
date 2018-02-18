/* [isd] igmp-keep-alive-server v0.1
 * =================================
 * Idea spawned by Kim Daniel @BF-SD7
 * Code by arnvid@isd.no - ievil/isd
 */


#include <getopt.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <string.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <termios.h>
#include <sys/poll.h>
#include <syslog.h>
#include <net/if.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <linux/if_packet.h>


char arg_interface[128];

void usage(char * binname) {
  printf("Usage: %s [-i interface]\n", binname);
  printf("  Default interface: lo\n");
  exit(1);
}


int main (int argc, char *argv[])
{
  time_t now, current, last;
  int fork_pid, real_pid, i, status;
  int sockfd, sockmc, mcast_dport, payload_len;
  struct sockaddr_in mcast_dst;
  struct sockaddr_in mcast_srv;
  struct ip_mreq mcast_req;
  char *mcast_ip;
  unsigned char mcast_ttl;
  char *payload;

//, done, upd_error;


  now = time(NULL);
  real_pid = getpid();
  strcpy(arg_interface, "eth0");
  mcast_ip = "239.0.0.1";
  mcast_dport = 1911;
  mcast_ttl = 1;
  payload = "Keep-Alive";
  payload_len = strlen(payload);

  openlog("igmp-keep-alive-server",LOG_PID, LOG_LOCAL5);
  syslog(LOG_NOTICE,"System starting up... initing... time: %li pid: %d!", now, real_pid);

  for (i=1;i<argc-1;++i) {
    if (!strcmp(argv[i], "-i")) {
      ++i;
      strncpy(arg_interface, argv[i], sizeof(arg_interface));
    } else {
      usage(argv[0]);
    }
  }
  fork_pid=fork();
  if (fork_pid<0) {
    printf("Error sending process to background.");
    syslog(LOG_CRIT, "System not forked successfully ... bailing.. system panic!");
    exit(1);
  }
  if (fork_pid) { exit(0); }
  close(0); close(1); close (2);
//  done = 1;
//  upd_error = 0;
  last = 0;

  if ((sockfd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
     syslog(LOG_CRIT, "Unable to open socket fd ... bailing.. system panic!");
     exit(1);
  }

  if ((sockmc = socket(PF_INET, SOCK_DGRAM, IPPROTO_IP)) == -1) {
     syslog(LOG_CRIT, "Unable to open socket fd ... bailing.. system panic!");
     exit(1);
  }


  memset(&mcast_dst, 0, sizeof(mcast_dst));
  memset(&mcast_req, 0, sizeof(struct ip_mreq));

  mcast_dst.sin_family = AF_INET;
  mcast_dst.sin_addr.s_addr = inet_addr(mcast_ip);
  mcast_dst.sin_port = htons(mcast_dport);

  mcast_srv.sin_family = PF_INET;
  mcast_srv.sin_port = htons(mcast_dport);
  mcast_srv.sin_addr.s_addr = htonl(INADDR_ANY);

  status = bind(sockmc, (struct sockaddr *)&mcast_srv, sizeof(struct sockaddr_in));
  if ( status < 0 ) {
      syslog(LOG_CRIT, "Unable to bind mcast server ... bailing.. system panic!");
     exit(1);
  }
  mcast_req.imr_multiaddr.s_addr = inet_addr("239.0.0.1");
  mcast_req.imr_interface.s_addr = INADDR_ANY;

  status = setsockopt(sockmc, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&mcast_req, sizeof(struct ip_mreq));

  if (setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_TTL, (void *) &mcast_ttl, sizeof(mcast_ttl)) < 0) {
     syslog(LOG_CRIT, "Unable to set socket options ... bailing.. system panic!");
     exit(1);
  }


  syslog(LOG_NOTICE, "System forked successfully, starting igmp keep alive server on %s.", arg_interface);
  do {
    current = time(NULL);
    if ( last+120 < current ) {
      syslog(LOG_NOTICE, "Sending IGMP Join for %s.", mcast_ip);
      status = setsockopt(sockmc, IPPROTO_IP, IP_ADD_MEMBERSHIP, (const void *)&mcast_req, sizeof(struct ip_mreq));
//      if ( status < 0 )
//        syslog(LOG_CRIT, "Failed to send IGMP Join ... bailing.. system panic!");
      syslog(LOG_NOTICE, "Sending IGMP Keep Alive to %s.", mcast_ip);
      last = time(NULL);
      if (sendto(sockfd, payload, payload_len, 0, (struct sockaddr *) &mcast_dst, sizeof(mcast_dst)) != payload_len) {
        syslog(LOG_CRIT, "Failed to send ... bailing.. system panic!");
        exit(1);
      }
      sleep(120);
    }
  } while(1);

  shutdown(sockmc, 2);
  close(sockmc);
  exit(0);
  return 0;                                     /* Keep some compilers happy */
}
