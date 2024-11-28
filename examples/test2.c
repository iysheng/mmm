/* small test showing a basic example of providing pcm data in a thread
 */
#include "mmm.h"
#include <stdio.h>
#include <signal.h>

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
static pthread_t gs_guilite_pt;

extern void startHelloWave(void* phy_fb, int width, int height, int color_bytes);

void * guilite_entry(void *parg)
{
    startHelloWave(parg, 800, 480, 4);
    return NULL;
}

void
handler(int signo, siginfo_t *info, void *context)
{
    struct sigaction oldact;

    printf("signo=%d\n", signo);
}

int main ()
{
    int frame = 1024;
    Mmm *fb = mmm_new (800, 480, 0, NULL);

    struct sigaction act = { 0 };

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = &handler;
    if (sigaction(SIGSEGV, &act, NULL) == -1)
    {
        perror("sigaction");
    }

    if (!fb)
    {
        fprintf (stderr, "failed to open buffer\n");
        return -1;
    }

    uint8_t *buffer;
    int width, height, stride;

    mmm_client_check_size (fb, &width, &height); /* does the real resize as a side-effect */

    buffer = mmm_get_buffer_write (fb, &width, &height, &stride, NULL);

    int ret;
    ret = pthread_create(&gs_guilite_pt, NULL, guilite_entry, buffer);
    pthread_detach(&gs_guilite_pt);

    while (1)
    {
        mmm_write_done (fb, 0, 0, -1, -1);
        frame++;

        while (mmm_has_event (fb))
        {
            const char *event = mmm_get_event (fb);
            fprintf (stderr, "%s\n", event);
        }
    }

    mmm_destroy (fb);
    return 0;
}
