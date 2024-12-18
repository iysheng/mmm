/* this is to serve as a minimal - no dependencies application
 * integrating with an mmm compositor - this example is minimal
 * enough that it doesn't even rely on the mmm .c file
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define WIDTH               800
#define HEIGHT              480
#define BPP                 4

#define MMM_PID             0x18
#define MMM_TITLE           0xb0
#define MMM_WIDTH           0x5b0
#define MMM_HEIGHT          0x5b4

#define MMM_DESIRED_WIDTH   0x5d8
#define MMM_DESIRED_HEIGHT  0x5dc

#define MMM_DAMAGE_WIDTH    0x5e8
#define MMM_DAMAGE_HEIGHT   0x5ec

#define MMM_STRIDE          0x5b8
#define MMM_FB_OFFSET       0x5bc
#define MMM_FLIP_STATE      0x5c0

#define MMM_SIZE            0x48c70
#define MMM_FLIP_INIT       0
#define MMM_FLIP_NEUTRAL    1
#define MMM_FLIP_DRAWING    2
#define MMM_FLIP_WAIT_FLIP  3
#define MMM_FLIP_FLIPPING   4

#define PEEK(addr)     (*(int32_t*)(&ram_base[addr]))
#define POKE(addr,val) do{(*(int32_t*)(&ram_base[addr]))=(val); }while(0)

static uint8_t *ram_base  = NULL;

static uint8_t *pico_fb (int width, int height)
{
  char path[800];
  int fd;
  int size = MMM_SIZE + width * height * BPP;
  const char *mmm_path = getenv ("MMM_PATH");
  if (!mmm_path)
  {
    fprintf (stderr, "this minimalistic demo requires to be launched as the argument of an mmm host being launched; for example $ mmm.sdl ./raw-client\n");
    /* copy over the fork launcher from lib? */
    return NULL;
  }
  sprintf (path, "%s/fb.XXXXXX", mmm_path);
  fd = mkstemp (path);
  pwrite (fd, "", 1, size);
  fsync (fd);
  chmod (path, 511);
  ram_base = mmap (NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  memset (ram_base, 0, size);
  strcpy ((void*)ram_base + MMM_TITLE, "MMM");

  POKE (MMM_FLIP_STATE,  MMM_FLIP_INIT);
  POKE (MMM_PID,         (uint32_t)getpid());

  POKE (MMM_WIDTH,          width);
  POKE (MMM_HEIGHT,         height);
  POKE (MMM_DESIRED_WIDTH,  width);
  POKE (MMM_DESIRED_HEIGHT, height);
  POKE (MMM_STRIDE,         width * BPP);

  POKE (MMM_FB_OFFSET,      MMM_SIZE); /* we write where to find out fb */
  POKE (MMM_FLIP_STATE,     MMM_FLIP_NEUTRAL);

  return (uint8_t*) & (PEEK(MMM_SIZE));
}

static void    pico_exit (void)
{
  if (ram_base)
    munmap (ram_base, MMM_SIZE * PEEK (MMM_WIDTH) * PEEK (MMM_HEIGHT) * BPP);
  ram_base = NULL;
}

static void wait_sync (void)
{
  /* this client will block if there is no compositor */
  while (PEEK(MMM_FLIP_STATE) != MMM_FLIP_NEUTRAL)
    usleep (5000);
  POKE (MMM_FLIP_STATE, MMM_FLIP_DRAWING);
}

static void flip_buffer (void)
{
  /* we need to indicate damage to the buffers to
   * get auto updates with hosts that are smart about updates
   */
  POKE (MMM_DAMAGE_WIDTH,  PEEK (MMM_WIDTH));
  POKE (MMM_DAMAGE_HEIGHT, PEEK (MMM_HEIGHT));

  /* indicate that we're ready for a flip */
  POKE (MMM_FLIP_STATE, MMM_FLIP_WAIT_FLIP);
}

#include <pthread.h>

static pthread_t gs_guilite_pt;

extern void startHelloWave(void* phy_fb, int width, int height, int color_bytes);

void * guilite_entry(void *parg)
{
  startHelloWave(parg, 800, 480, 4);
  
  return 0;
}

int main (int argc, char **argv)
{
  // 默认是 ARGB format
  uint8_t *pixels = pico_fb (800, 480);

  if (!pixels)
    return -1;

  // TODO create new thread
  int ret;
  ret = pthread_create(&gs_guilite_pt, NULL, guilite_entry, pixels);
  pthread_detach(&gs_guilite_pt);

  // update fb
  while(1)
  {
    wait_sync ();
    flip_buffer();
  }

  pico_exit ();
  return 0;
}
