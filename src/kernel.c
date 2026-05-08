#include <stdint.h>
#include <stddef.h>
#include "limine.h"

// Limineへのメモリマップ要求
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

void _start(void) {
    // リクエストが成功したかチェック
    if (memmap_request.response == NULL) {
        for (;;);
    }

    // エントリ数を確認
    uint64_t entry_count = memmap_request.response->entry_count;
    
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        
        // entry->type が LIMINE_MEMMAP_USABLE なら、そこは自由に使っていい場所です
        // entry->base : 開始アドレス
        // entry->length : サイズ
    }

    for (;;);
}
