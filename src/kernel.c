#include "malloc.h"
#include <stdint.h>
#include <stddef.h>
#include "limine.h"

struct heap_chunk; // 構造体の前方宣言
extern struct heap_chunk* heap_start;

// --- リクエスト群 ---
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

struct heap_chunk {
    size_t size;
    struct heap_chunk* next;
    int free;
};
extern struct heap_chunk* heap_start;

// --- PMM管理変数 ---
uint8_t* bitmap;
uint64_t bitmap_size;
uintptr_t max_address = 0;
uint64_t total_usable_memory = 0;

// --- ビット操作関数 ---
void bitmap_set(uint64_t index) {
    bitmap[index / 8] |= (1 << (index % 8));
}

void bitmap_clear(uint64_t index) {
    bitmap[index / 8] &= ~(1 << (index % 8));
}

// --- ステップ2 & 3: PMM初期化 ---
void pmm_init(void) {
    uint64_t total_pages = max_address / 4096;
    bitmap_size = total_pages / 8;

    // ビットマップの配置場所を決定
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE && entry->length >= bitmap_size) {
            bitmap = (uint8_t*)(entry->base + hhdm_request.response->offset);
            
            // 一旦すべてを「使用中(1)」にする
            for (uint64_t j = 0; j < bitmap_size; j++) {
                bitmap[j] = 0xff;
            }
            break;
        }
    }

    // ステップ3: USABLEなページを「空き(0)」として登録
    for (uint64_t i = 0; i < memmap_request.response->entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];

        if (entry->type == LIMINE_MEMMAP_USABLE) {
            for (uint64_t j = 0; j < entry->length; j += 4096) {
                // そのページのインデックスを計算してクリア
                bitmap_clear((entry->base + j) / 4096);
            }
        }
    }

    // 重要：ビットマップ自身が置かれている領域を「使用中」に戻す
    // そうしないと、pmm_allocがビットマップ領域を他人に貸し出して壊してしまいます
    uintptr_t bitmap_phys = (uintptr_t)bitmap - hhdm_request.response->offset;
    for (uint64_t j = 0; j < bitmap_size; j += 4096) {
        bitmap_set((bitmap_phys + j) / 4096);
    }
}

void _start(void) {
    if (memmap_request.response == NULL || hhdm_request.response == NULL) {
        for (;;);
    }

    // 1. まずメモリを解析して max_address を確定させる
    uint64_t entry_count = memmap_request.response->entry_count;
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            total_usable_memory += entry->length;
            uintptr_t top_address = entry->base + entry->length;
            if (top_address > max_address) {
                max_address = top_address;
            }
        }
    }

    // 2. その後にPMMを初期化
    pmm_init();

    // これで PMM の初期化が完了！
    // 次は pmm_alloc() を作れば、メモリの切り出しが可能になります。

    for (;;);
}

// 次回検索を開始する位置を覚えておくと少し速くなります（Last Fit）
uint64_t last_index = 0;

void* pmm_alloc(void) {
    uint64_t total_pages = max_address / 4096;

    for (uint64_t i = 0; i < total_pages; i++) {
        // last_index から検索を開始して効率化
        uint64_t index = (last_index + i) % total_pages;

        if (bitmap[index / 8] != 0xff) { // そのバイトに空き(0)が1つ以上あるか
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (!((bitmap[index / 8] >> bit) & 1)) {
                    // 空きビット発見！
                    uint64_t final_index = (index / 8) * 8 + bit;
                    bitmap_set(final_index); // 使用中にする
                    last_index = final_index; // 次回のために場所を記録

                    // 物理アドレスを仮想アドレス(HHDM)に変換して返す
                    uintptr_t phys_addr = final_index * 4096;
                    return (void*)(phys_addr + hhdm_request.response->offset);
                }
            }
        }
    }

    return NULL; // メモリ不足（Out of Memory）
}

void pmm_free(void* ptr) {
    if (ptr == NULL) return;

    // 仮想アドレス(HHDM)を物理アドレスに戻す
    uintptr_t virt_addr = (uintptr_t)ptr;
    uintptr_t phys_addr = virt_addr - hhdm_request.response->offset;

    // インデックスを計算してビットをクリア
    uint64_t index = phys_addr / 4096;
    bitmap_clear(index);
}

void* kmalloc(size_t size) {
    if (size == 0) return NULL;

    size = ALIGN(size); // 要求サイズを16バイト境界に整列
    struct heap_chunk* current = heap_start;

    // 適切な空きブロックを探す
    while (current) {
        if (current->free && current->size >= size) {
            
            // 分割の判定：残りカスがヘッダー＋最小単位（16Byte）より大きければ分割
            if (current->size >= (size + HEADER_SIZE + 16)) {
                // 新しい空きブロック（後半部分）を作成
                struct heap_chunk* next_chunk = (struct heap_chunk*)((uint8_t*)current + HEADER_SIZE + size);
                next_chunk->size = current->size - size - HEADER_SIZE;
                next_chunk->next = current->next;
                next_chunk->free = 1;

                // 現在のブロックを更新
                current->size = size;
                current->next = next_chunk;
            }

            current->free = 0; // 使用中にマーク
            
            // ヘッダーの直後のアドレス（実際のデータ領域）を返す
            return (void*)((uint8_t*)current + HEADER_SIZE);
        }
        current = current->next;
    }

    // ここでNULLが返る場合は、本来「PMMから新しいページを借りてヒープを広げる」処理が必要
    return NULL; 
}

void kfree(void* ptr) {
    if (ptr == NULL) return;

    // データ領域のアドレスからヘッダーのアドレスを逆算
    struct heap_chunk* chunk = (struct heap_chunk*)((uint8_t*)ptr - HEADER_SIZE);
    
    // ブロックを空き状態にする
    chunk->free = 1;

    // 隣接するブロックとの結合（Coalescing）
    // 次のブロックが空いていれば、今のブロックに吸収させる
    struct heap_chunk* current = heap_start;
    while (current) {
        if (current->free && current->next && current->next->free) {
            // 現在のサイズ = 現在のデータサイズ + ヘッダーサイズ + 次のブロックのデータサイズ
            current->size += HEADER_SIZE + current->next->size;
            // リンクを一つ飛ばす
            current->next = current->next->next;
            
            // 結合した結果、さらにその次も結合できる可能性があるので
            // current を進めずに再度チェックする
            continue; 
        }
        current = current->next;
    }
}
