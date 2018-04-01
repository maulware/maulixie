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

int main(int argc, char **argv)
{
  char buffer[BUFSIZ];
  char protoname[] = "tcp";
  struct protoent *protoent;
  char *server_hostname = "94.45.232.48";
  in_addr_t in_addr;
  in_addr_t server_addr;
  int sockfd;
  size_t getline_buffer = 0;
  /* This is the struct used by INet addresses. */
  struct sockaddr_in sockaddr_in;
  unsigned short server_port = 1234;

  int width, height;
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *row_pointers;


  FILE *fp = fopen("/home/mauli/Pictures/vector_laughing_man_logo_by_ericdbz.png", "rb");
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

  //now use the file data

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

  inet_pton(AF_INET,"94.45.232.48", &(sockaddr_in.sin_addr));
  sockaddr_in.sin_port=htons(server_port);
  sockaddr_in.sin_family=AF_INET;

  if (connect(sockfd, (struct sockaddr*)&sockaddr_in, sizeof(sockaddr_in)) == -1) {
      perror("connect");
      return EXIT_FAILURE;
  }

  sleep(1);

  int xmax=width;
  int ymax=height;
  //char arr[xmax+1][ymax+1][42];
//  char arr[200][700][42];

printf("Size: %d %d\n",xmax,ymax);

  for(int y = 0; y < height; y++) {
//    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
//      png_bytep px = &(row[x * 4]);
      // Do something awesome for each pixel here...
//      printf("%4d, %4d = RGBA(%3d, %3d, %3d, %3d)\n", x, y, px[0], px[1], px[2], px[3]);
      
//      sprintf(arr[x][y],"PX %d %d %02x%02x%02x\n",x,y,px[0], px[1], px[2]);
//      printf("%d %d: %s\n",x,y,arr[x][y]);
    }
  }

//  exit;

  char pri[100];
  png_bytep row;
  png_bytep px;
  sprintf(pri,"OFFSET 0 0\n");
  send(sockfd, pri, strlen(pri), 0);
  while(1)
  {
    #pragma omp parallel for schedule(dynamic,1) collapse(2)
    for(int y=0;y<ymax;y++){
      for(int x=0;x<xmax;x++){
        row = row_pointers[y];
        px = &(row[x * 4]);
        sprintf(pri,"PX %d %d %02x%02x%02x\n",x,y,px[0], px[1], px[2]);
        send(sockfd, pri, strlen(pri), 0);
      }
    }
  }
}
