#include "../../apps.h"

#include "lvgl/lvgl.h"
#include "pngle/pngle.h"
#include "comic.h"

#include <wolfssl/ssl.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <netdb.h>

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>

typedef struct {
    lv_obj_t * canv;
    bool done;
} pstate_t;

static void init_cb(pngle_t *dec, uint32_t w, uint32_t h) {
    pstate_t * ps = pngle_get_user_data(dec);
    assert(lv_canvas_get_buf(ps->canv) == NULL);
    void * buf = malloc(w * h * 4);
    assert(buf);
    lv_canvas_set_buffer(ps->canv, buf, w, h, LV_COLOR_FORMAT_ARGB8888);
}

static void draw_cb(pngle_t *dec, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]) {
    (void)w;
    (void)h;
    pstate_t * ps = pngle_get_user_data(dec);
    assert(lv_canvas_get_buf(ps->canv) != NULL);
    lv_canvas_set_px(ps->canv, x, y, lv_color_make(rgba[0], rgba[1], rgba[2]), rgba[3]);
}

static void done_cb(pngle_t *dec) {
    pstate_t * ps = pngle_get_user_data(dec);
    ps->done = true;
}

typedef struct {
    WOLFSSL_CTX * ctx;
    WOLFSSL * ssl;
    int fd;
    const char * err;
} https_connect_ret_t;

static void https_connect(https_connect_ret_t * ret, const char * domain)
{
    ret->err = NULL;

    int intret;

    // ssl initialization.
    if(wolfSSL_Init() != SSL_SUCCESS)
    {
        ret->err = "wolfSSL_Init() error";
        return;
    }

    //init context
    WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfTLS_client_method());
    if (!ctx)
    {
        ret->err = "cannot create SSL context";
        return;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((intret = wolfSSL_CTX_load_verify_buffer_ex(ctx, ca_cert_der_2048,
            sizeof_ca_cert_der_2048, SSL_FILETYPE_PEM, 0, WOLFSSL_LOAD_FLAG_DATE_ERR_OKAY)) != WOLFSSL_SUCCESS) {
        printf("ERROR: failed to load CA cert. %d\n", intret);
        return;
    }

    /* code taken from tcp_client_classic.c */
    struct addrinfo hints;

    memset(&hints, 0, sizeof(hints));

    struct addrinfo *peer;
    hints.ai_socktype = SOCK_STREAM;

    if (0 != (intret = getaddrinfo(domain, "443", &hints, &peer)))
    {
        ret->err = gai_strerror(intret);
        return;
    }

    char peer_addr[50];
    char peer_protocol[50];
    if(0 != getnameinfo(peer->ai_addr, peer->ai_addrlen, peer_addr, sizeof(peer_addr), peer_protocol, sizeof(peer_protocol), NI_NUMERICHOST))
    {
        ret->err = "getnameinfo() error";
        return;
    }

    //family socket_type protocol
    int socket_fd = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);

    if (socket_fd < 0)
    {
        ret->err = "socket error";
        return;
    }

    if (connect(socket_fd, peer->ai_addr, peer->ai_addrlen) < 0)
    {
        ret->err = "connect error";
        return;
    }

    /* END */

    WOLFSSL *ssl = wolfSSL_new(ctx);
    if (!ssl)
    {
        ret->err = "SSL_new() failed";
        return;
    }

    // if (!wolfSSL_set_tlsext_host_name(ssl, domain))
    // {
    //     ret->err = "SSL_set_tlsext_host_name() failed";
    //     return;
    // }

    if (wolfSSL_set_fd(ssl, socket_fd) != SSL_SUCCESS)
    {
        ret->err = "SSL_set_fd() failed";
        return;
    }

    if ((intret = wolfSSL_connect(ssl)) != SSL_SUCCESS)
    {
        int err = wolfSSL_get_error(ssl, intret);
        char buf[100] = {0};
        wolfSSL_ERR_error_string_n(err, buf, 99);
        printf("%s\n", buf);
        ret->err = "SSL_connect() failed";
        return;
    }

    ret->ctx = ctx;
    ret->ssl = ssl;
    ret->fd = socket_fd;
}

void xkcd_app(void)
{
    pngle_t * dec = pngle_new();
    assert(dec);

    pstate_t * ps = malloc(sizeof(pstate_t));
    assert(ps);
    ps->canv = lv_canvas_create(lv_screen_active());
    ps->done = false;

    pngle_set_user_data(dec, ps);
    pngle_set_init_callback(dec, init_cb);
    pngle_set_draw_callback(dec, draw_cb);
    pngle_set_done_callback(dec, done_cb);

    // for(size_t i = 0; i < sizeof(comic_bin) && !ps->done; i += 1024) {
    //     int ret = pngle_feed(dec, comic_bin + i, 1024);
    //     assert(ret == 1024);
    // }

    assert(!ps->done);
    int ret = pngle_feed(dec, comic_bin, sizeof(comic_bin));
    assert(ret == sizeof(comic_bin));
    assert(ps->done);

    pngle_destroy(dec);
    // void * buf = lv_canvas_get_buf(ps->canv);
    // lv_obj_delete(ps->canv);
    // free(buf);
    free(ps);

    ///////////////////////////////////////////////

    https_connect_ret_t conn_ret;
    https_connect(&conn_ret, "xkcd.com");
    printf("con ret err: %s\n", conn_ret.err ? conn_ret.err : "no error");
}
