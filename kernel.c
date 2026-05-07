#include <stdint.h>
#include <stddef.h>
#include "limine.h"

// Limineプロトコルでフレームバッファを要求
static volatile struct limine_framebuffer_request fb_req = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0
};

// カーネルのエントリポイント
void _start(void) {
    // レスポンスが正常か確認
    if (fb_req.response == NULL || fb_req.response->framebuffer_count < 1) {
        while (1) { __asm__("hlt"); }
    }

    struct limine_framebuffer *fb = fb_req.response->framebuffers[0];

    // 画面全体を「Aura」をイメージした色で塗りつぶす
    uint32_t *fb_ptr = (uint32_t *)fb->address;
    for (size_t i = 0; i < fb->width * fb->height; i++) {
        fb_ptr[i] = 0x2E3440; // 深みのあるモダンなグレー（Nordテーマ風）
    }

    // CPUを停止させて待機
    while (1) { __asm__("hlt"); }
}
