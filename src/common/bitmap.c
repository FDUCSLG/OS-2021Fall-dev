#include <common/bitmap.h>
#include <common/string.h>

void init_bitmap(BitmapCell *bitmap, usize num_bits) {
    memset(bitmap, 0, BITMAP_TO_NUM_CELLS(num_bits) * sizeof(BitmapCell));
}
