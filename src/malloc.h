#ifndef MALLOC_H
#define MALLOC_H

#include <stddef.h>
#include <stdint.h>

struct heap_chunk {
    size_t size;
    struct heap_chunk* next;
    int free;
};

// 全ファイル共通で使う「実体は外にあるよ」という宣言
extern struct heap_chunk* heap_start;

#define ALIGN(size) (((size) + (16 - 1)) & ~(16 - 1))
#define HEADER_SIZE sizeof(struct heap_chunk)

void heap_init(void);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
