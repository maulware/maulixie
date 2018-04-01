#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>

#include <arpa/inet.h>
#include <netdb.h> /* getprotobyname */
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string.h>

#include <omp.h>
#include <png.h>
#include <pthread.h>

char buffer[BUFSIZ];
char protoname[] = "tcp";
char *server_hostname = "94.45.232.48";
unsigned short server_port = 1234;

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers;

void openFile(char *file)
{

  FILE *fp = fopen(file, "rb");
//THX: https://gist.github.com/niw/5963798
  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);
  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth = png_get_bit_depth(png, info);

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);
  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);
  fclose(fp);

}

void *flood(void *param)
{
  struct protoent *protoent;
  in_addr_t in_addr;
  in_addr_t server_addr;
  int sockfd;
  size_t getline_buffer = 0;
  /* This is the struct used by INet addresses. */
  struct sockaddr_in sockaddr_in;

  char *ip=(char*) param;

  printf("starting connection via %s\n", ip);

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
  //localaddr.sin_addr.s_addr = inet_addr(argv[0]);
  inet_pton(AF_INET, ip, &(localaddr.sin_addr));
  localaddr.sin_port = 0;  // Any local port will do
  if (bind(sockfd, (struct sockaddr *)&localaddr, sizeof(localaddr)) == -1) {
    perror("bind");
    return EXIT_FAILURE;
  }

  inet_pton(AF_INET, server_hostname, &(sockaddr_in.sin_addr));
  sockaddr_in.sin_port=htons(server_port);
  sockaddr_in.sin_family=AF_INET;

  if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
      perror("connect");
      return EXIT_FAILURE;
  }

  sleep(1);

  int xmax=width;
  int ymax=height;

printf("Size: %d %d\n",xmax,ymax);

  char pri[100];
  png_bytep row;
  png_bytep px;
  sprintf(pri,"OFFSET 300 100\n");
  send(sockfd, pri, strlen(pri), 0);

  int p_step=4;
  while(1)
  {
  for(int p=0;p<p_step;p++){
//    #pragma omp parallel for schedule(dynamic,1) collapse(2)
    for(int y=p;y<ymax;y+=p_step){
      for(int x=p;x<xmax;x+=p_step){
          row = row_pointers[y];
          px = &(row[x * 4]);
          //ignore pic black
	  if(px[0]==0x05 && px[1] == 0x07 && px[2] == 0x07)
            continue;
          //ignore full white
//	  if(px[0]==255 && px[1] == 255 && px[2] == 255)
//            continue;
          //ignore blue 084c74
	  if(px[0]==0x08 && px[1] == 0x4c && px[2] == 0x74)
            continue;
	  
 	  sprintf(pri,"PX %d %d %02x%02x%02x\n",x,y,px[0], px[1], px[2]);
          send(sockfd, pri, strlen(pri), MSG_NOSIGNAL);
        }
      }
    }
  }
}

int main(int argc, char **argv)
{

  openFile("/home/mauli/Pictures/vector_laughing_man_logo_by_ericdbz.png");

  pthread_t tid;
  pthread_t tids[argc-1];

  printf("Starting %i threads\n", argc-1);
  for(int i=1; i<argc-1; i++){
    pthread_create(&tid, NULL, flood, (void *)argv[i+1]);
    tids[i]=tid;
  }

  flood(argv[1]);  

}
