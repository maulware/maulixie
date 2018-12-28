//#define _XOPEN_SOURCE 700

#include <arpa/inet.h>
#include <assert.h>
#include <getopt.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */

#include <omp.h>
#include <png.h>
#include <pthread.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

int numips = 0;
int verbose_flag = 0;
char *file = 0;
char *ip = 0;
int port = 0;
int limit = 0;
int offx=0;
int offy=0;

// just set to 100
struct in_addr local_ips[100];
struct in_addr *getIPs(void) {
  // thx:
  // https://stackoverflow.com/questions/1160963/how-to-enumerate-all-ip-addresses-attached-to-a-machine-in-posix-c/1161018#1161018
  static struct ifreq ifreqs[32];
  struct ifconf ifconf;

  memset(&ifconf, 0, sizeof(ifconf));
  ifconf.ifc_req = ifreqs;
  ifconf.ifc_len = sizeof(ifreqs);

  int sd = socket(PF_INET, SOCK_STREAM, 0);
  assert(sd >= 0);

  int r = ioctl(sd, SIOCGIFCONF, (char *)&ifconf);
  assert(r == 0);

  numips = ifconf.ifc_len / sizeof(struct ifreq);

  printf("got %d interfaces\n", numips);
  for (int i = 0; i < numips; ++i) {
    printf("%s: %s\n", ifreqs[i].ifr_name,
           inet_ntoa(((struct sockaddr_in *)&ifreqs[i].ifr_addr)->sin_addr));
    local_ips[i] = ((struct sockaddr_in *)&ifreqs[i].ifr_addr)->sin_addr;
  }

  close(sd);

  return local_ips;
}

char buffer[BUFSIZ];
char protoname[] = "tcp";
char *server_hostname;
unsigned short server_port;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;

int openFile(char *file) {

  FILE *fp = fopen(file, "rb");
  // THX: https://gist.github.com/niw/5963798
  png_structp png =
      png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if (!png)
    abort();

  png_infop info = png_create_info_struct(png);
  if (!info)
    abort();

  if (setjmp(png_jmpbuf(png)))
    abort();

  png_init_io(png, fp);
  png_read_info(png, info);

  width = png_get_image_width(png, info);
  height = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);

  printf("Picture is %d x %d\n",width,height);

  if (bit_depth == 16)
    png_set_strip_16(png);

  if (color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if (png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if (color_type == PNG_COLOR_TYPE_RGB || color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);
  row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * height);
  for (int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte *)malloc(png_get_rowbytes(png, info));
  }

  png_read_image(png, row_pointers);
  fclose(fp);

  return 0;
}

int *flood(struct in_addr *ip) {
  struct protoent *protoent;
  in_addr_t in_addr;
  in_addr_t server_addr;
  int sockfd;
  size_t getline_buffer = 0;
  /* This is the struct used by INet addresses. */
  struct sockaddr_in sockaddr_in;

  printf("starting connection via %s\n", inet_ntoa(*ip));

  protoent = getprotobyname(protoname);
  if (protoent == NULL) {
    perror("getprotobyname");
    exit(EXIT_FAILURE);
  }
  sockfd = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
  if (sockfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Bind to a specific network interface (and optionally a specific local port)
  struct sockaddr_in localaddr;
  localaddr.sin_family = AF_INET;
  localaddr.sin_addr = *ip;
  localaddr.sin_port = 0; // Any local port will do
  if (bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) == -1) {
    perror("bind");
    EXIT_FAILURE;
  }

  inet_pton(AF_INET, server_hostname, &(sockaddr_in.sin_addr));
  sockaddr_in.sin_port = htons(server_port);
  sockaddr_in.sin_family = AF_INET;

  if (connect(sockfd, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) ==
      -1) {
    perror("connect");
    EXIT_FAILURE;
  }

  sleep(1);

  int xmax = width;
  int ymax = height;

  char pri[100];
  png_bytep row;
  png_bytep px;
  sprintf(pri, "OFFSET 20 20\n");
  send(sockfd, pri, strlen(pri), 0);

  int p_step = 4;
  while (1) {
    for (int p_x = 0; p_x < p_step; p_x++) {
      for (int p_y = 0; p_y < p_step; p_y++) {
        //    #pragma omp parallel for schedule(dynamic,1) collapse(2)
        for (int y = p_y; y < ymax; y += p_step) {
          for (int x = p_x; x < xmax; x += p_step) {
            row = row_pointers[y];
            px = &(row[x * 4]);
            // ignore pic black
            if (px[0] == 0x05 && px[1] == 0x07 && px[2] == 0x07)
              continue;
            // ignore full white
            //	  if(px[0]==255 && px[1] == 255 && px[2] == 255)
            //            continue;
            // ignore blue 084c74
            //	  if(px[0]==0x08 && px[1] == 0x4c && px[2] == 0x74)
            //            continue;

            sprintf(pri, "PX %d %d %02x%02x%02x\n", x+offx, y+offy, px[0], px[1], px[2]);
            send(sockfd, pri, strlen(pri), MSG_NOSIGNAL);
          }
        }
      }
    }
  }
}

int parseArguments(int argc, char **argv) {
  int c;

  while (1) {
    static struct option long_options[] = {
        /* These options set a flag. */
        {"verbose", no_argument, &verbose_flag, 1},
        {"brief", no_argument, &verbose_flag, 0},
        /* These options don’t set a flag.
           We distinguish them by their indices. */
        {"limit", required_argument, 0, 'l'},
        {"ip", required_argument,    0, 'i'},
        {"port", required_argument,  0, 'p'},
        {"file", required_argument,  0, 'f'},
        {"offx", required_argument,  0, 'x'},
        {"offy", required_argument,  0, 'y'},
        {0, 0, 0, 0}};
    /* getopt_long stores the option index here. */
    int option_index = 0;

    c = getopt_long(argc, argv, "abc:x:y:l:d:f:", long_options, &option_index);

    /* Detect the end of the options. */
    if (c == -1)
      break;

    switch (c) {
    case 0:
      /* If this option set a flag, do nothing else now. */
      if (long_options[option_index].flag != 0)
        break;
      printf("option %s", long_options[option_index].name);
      if (optarg)
        printf(" with arg %s", optarg);
      printf("\n");
      break;

    case 'l':
      printf("option -l limit with value `%s'\n", optarg);
      limit = atoi(optarg);
      break;

    case 'f':
      printf("option -f file with value `%s'\n", optarg);
      file = optarg;
      break;

    case 'i':
      printf("option -i ip with value `%s'\n", optarg);
      ip = server_hostname = optarg;
      break;

    case 'p':
      printf("option -p port with value `%s'\n", optarg);
      port = server_port = atoi(optarg);
      break;

    case 'x':
      printf("option -x offx with value `%s'\n", optarg);
      offx = atoi(optarg);
      break;

    case 'y':
      printf("option -y offy with value `%s'\n", optarg);
      offy = atoi(optarg);
      break;

    case '?':
      /* getopt_long already printed an error message. */
      break;

    default:
      abort();
    }
  }

  /* Instead of reporting ‘--verbose’
     and ‘--brief’ as they are encountered,
     we report the final status resulting from them. */
  if (verbose_flag)
    puts("verbose flag is set");

  /* Print any remaining command line arguments (not options). */
  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    putchar('\n');
  }

  return 0;
}

int main(int argc, char **argv) {

  char *picture = argv[1];

  if (parseArguments(argc, argv) != 0i || file == 0 || ip == 0|| port == 0) {
    printf("Parse errors, please read, thx & bye\n");
    printf("please add at least: file, ip, port\n");
    return 1;
  } else {
    printf("params read, thx\n");
  }

  getIPs();

  int err = openFile(file);

  if (err != 0) {
    return EXIT_FAILURE;
  }

  static int maxthreads=100;
  pthread_t tid;
  pthread_t tids[maxthreads];
  int useThreads = MIN(MIN(numips, limit),maxthreads);

  printf("Starting %i threads\n", useThreads);
  for (int i = 0; i < useThreads; i++) {
    pthread_create(&tid, NULL, flood, &local_ips[i]);
    tids[i] = tid;
    sleep(1);
  }

  for (int i = 0; i < useThreads; i++)
    pthread_join(tids[i], NULL);

  printf("All threads finished ... died\n");
}
