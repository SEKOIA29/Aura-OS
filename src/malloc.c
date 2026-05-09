#include "malloc.h"

// メモリブロックのヘッダー
struct heap_chunk {
    size_t size;                // このブロックのデータ部分のサイズ
    struct heap_chunk* next;    // 次のブロックへのポインタ
    int free;                   // 1なら空き、0なら使用中
};

// リストの先頭
struct heap_chunk* heap_start = NULL;

// アライメント（16バイト境界に合わせるとCPUが喜びます）
#define ALIGN(size) (((size) + (16 - 1)) & ~(16 - 1))
#define HEADER_SIZE sizeof(struct heap_chunk)


// PMMからページをもらうためのプロトタイプ宣言（kernel.cにある想定）
extern void* pmm_alloc(void);

void heap_init() {
    // 1. PMMから最初の1ページ(4KB)を借りる
    void* initial_page = pmm_alloc();
    if (initial_page == NULL) return;

    // 2. そのページの先頭に、最初の「巨大な空きブロック」を作る
    heap_start = (struct heap_chunk*)initial_page;
    heap_start->size = 4096 - HEADER_SIZE; // ページ全体からヘッダー分を引いた残り
    heap_start->next = NULL;
    heap_start->free = 1; // 最初は全部空いている
}
