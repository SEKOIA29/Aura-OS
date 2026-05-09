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
