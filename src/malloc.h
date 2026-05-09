#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>
#include <stdint.h>

// メモリブロックのヘッダー
struct heap_chunk {
    size_t size;
    struct heap_chunk* next;
    int free;
};

// アライメントとサイズ定数
#define ALIGN(size) (((size) + (16 - 1)) & ~(16 - 1))
#define HEADER_SIZE sizeof(struct heap_chunk)

// 関数宣言
void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
