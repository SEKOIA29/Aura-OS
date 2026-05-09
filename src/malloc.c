#include "malloc.h"

// リストの先頭（実体の定義：externは付けない）
struct heap_chunk* heap_start = NULL;

// PMMからページをもらうためのプロトタイプ宣言
extern void* pmm_alloc(void);

// ヒープの初期化
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
