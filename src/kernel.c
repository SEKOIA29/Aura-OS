#include <stdint.h>
#include <stddef.h>
#include "limine.h"

// Limineへのメモリマップ要求（ビルド時にこの情報を入れるようブートローダーに伝えます）
static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0
};

// PMMの状態を保持する変数（後のステップで使用）
uint64_t total_usable_memory = 0;
uintptr_t max_address = 0;

void _start(void) {
    // 1. リクエストが成功したかチェック
    if (memmap_request.response == NULL) {
        // 失敗した場合は停止（本来はここでエラー処理）
        for (;;);
    }

    // 2. メモリマップを走査して解析
    uint64_t entry_count = memmap_request.response->entry_count;
    
    for (uint64_t i = 0; i < entry_count; i++) {
        struct limine_memmap_entry *entry = memmap_request.response->entries[i];
        
        // 自由に使えるメモリ（USABLE）だけを探す
        if (entry->type == LIMINE_MEMMAP_USABLE) {
            // 合計容量を加算
            total_usable_memory += entry->length;
            
            // 最大物理アドレスを更新
            uintptr_t top_address = entry->base + entry->length;
            if (top_address > max_address) {
                max_address = top_address;
            }
        }
    }

    // ここまでで「何バイト使えるか」と「メモリの終端がどこか」が判明しました
    // 次はこの max_address を元に、ビットマップのサイズを計算します。

    for (;;);
}
