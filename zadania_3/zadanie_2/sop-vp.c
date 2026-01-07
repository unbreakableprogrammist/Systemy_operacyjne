#include "video-player.h"

// TODO in stage 3
typedef struct circular_buffer
{
} circular_buffer;

// circular_buffer* circular_buffer_create() {}
// void circular_buffer_push(circular_buffer* buffer, video_frame* frame) {}
// video_frame* circular_buffer_pop(circular_buffer* buffer) {}
// void circular_buffer_destroy(circular_buffer* buffer) {}

int main(int argc, char* argv[])
{
    UNUSED(argc);
    UNUSED(argv);

    // one-thread video_frame flow example. You can remove those lines
    video_frame* f = decode_frame();
    transform_frame(f);
    display_frame(f);

    exit(EXIT_SUCCESS);
}
