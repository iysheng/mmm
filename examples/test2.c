/* small test showing a basic example of providing pcm data in a thread
 */
#include "mmm.h"
#include <stdio.h>
#include <stdbool.h>
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

void handler(int signo, siginfo_t *info, void *context)
{
    struct sigaction oldact;

    printf("signo=%d\n", signo);
}

enum
{
    EVENT_PUSH,
    EVENT_RELEASE,
};

int convert_event2touch(char *buffer, int *x, int *y)
{
    int ret = -1;
    if (buffer == strstr(buffer, "mouse-press"))
    {
        sscanf(buffer, "%*[a-z|-] %d %d", x, y);
        ret = EVENT_PUSH;
    }
    else if (buffer == strstr(buffer, "mouse-release"))
    {
        sscanf(buffer, "%*[a-z|-] %d %d", x, y);
        ret = EVENT_RELEASE;
    }

    return ret;
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

    int x, y;
    while (1)
    {
        mmm_write_done (fb, 0, 0, -1, -1);
        frame++;

        while (mmm_has_event (fb))
        {
            const char *event = mmm_get_event (fb);
            ret = convert_event2touch(event, &x, &y);
            fprintf (stderr, "%s ret=%d x=%d y=%d\n", event, ret, x, y);
            switch (ret)
            {
            case EVENT_PUSH:
                sendTouch2HelloWave(x, y, true);
                break;
            case EVENT_RELEASE:
                sendTouch2HelloWave(x, y, false);
                break;
            default:
                break;
            }
        }
    }

    mmm_destroy (fb);
    return 0;
}
