#include <stdint.h>
#include <stddef.h>
#include "limine.h"

// --- リクエスト群 ---
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0
};

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
