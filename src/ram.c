#include "ram.h"
#include <stdint.h>
#include <stddef.h>
#include <string.h>  // for memset
#include <stdio.h>   // for debugging

static tRam *ram = NULL;  // Pointer to RAM metadata

// Helper: check if n is a power of two
static int is_power_of_two(uint32_t n) {
    return n && ((n & (n - 1)) == 0);
}

int init_ram(void *memory, uint16_t size, uint8_t page_size) {
    if (!memory) return -3;
    if (size == 0 || !is_power_of_two(size)) return -1;
    if (page_size == 0 || !is_power_of_two(page_size) || page_size > size) return -2;

    // Check memory is zeroed
    uint8_t *mem = (uint8_t *)memory;
    for (uint16_t i = 0; i < size; i++) {
        if (mem[i] != 0) return -3;
    }

    // Compute number of frames
    uint16_t num_frames = size / page_size;
    if (num_frames == 0) return -2;

    // Check space for tRam struct + bitmap
    size_t bitmap_bytes = (num_frames + 7) / 8; // 1 bit per frame
    size_t metadata_size = sizeof(tRam) + bitmap_bytes;


    if (metadata_size > size) return -4;

    ram = (tRam *)memory;
    ram->size = size;
    ram->page_size = page_size;
    ram->bitmap = (uint8_t *)memory + sizeof(tRam);

    // Zero bitmap
    memset(ram->bitmap, 0, bitmap_bytes);

    uint16_t metadata_frames = (metadata_size + page_size - 1) / page_size;
    for (uint16_t i = 0; i < metadata_frames; i++) {
        ram->bitmap[i / 8] |= 1 << (i % 8);
    }

    return num_frames;
}

void destroy_ram() {
    ram = NULL; // RAM is still in memory, just clear metadata pointer
}

int falloc(uint16_t *frame_id, uint16_t number){
    if (!ram) return -1;
    if (!frame_id || number == 0) return -2;

    uint16_t num_frames = ram->size / ram->page_size;
    uint16_t start = 0;
    uint16_t count = 0;

    for (uint16_t i = 0; i < num_frames; i++) {
        uint8_t bit = 1 << (i % 8);
        if ((ram->bitmap[i / 8] & bit) == 0) { // check live bitmap
            if (count == 0) start = i;
            count++;
            if (count == number) {
                 // mark frames as used
                 for (uint16_t j = start; j < start + number; j++) {
                     ram->bitmap[j / 8] |= 1 << (j % 8);
                 }
                 *frame_id = start;
                 return 0;
            }
        } else {
            count = 0; // reset count
        }
    }

    return -1;
}


void ffree(uint16_t frame_id, uint16_t number) {
    if (!ram) return;

    uint16_t num_frames = ram->size / ram->page_size;
    if (frame_id >= num_frames) return;

    if (frame_id + number > num_frames) number = num_frames - frame_id;

    for (uint16_t i = frame_id; i < frame_id + number; i++) {
        ram->bitmap[i / 8] &= ~(1 << (i % 8));
    }
}

const tRam *get_ram_state() {
    return ram;
}
