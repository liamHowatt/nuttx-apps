#include "../../apps.h"

#include "lvgl/lvgl.h"
#include "pngle/pngle.h"
#include "comic.h"

#include <wolfssl/wolfcrypt/settings.h>
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

#define HOST "matrix.beeper.com"

typedef struct {
    lv_obj_t * canv;
    bool done;
} pstate_t;

static const unsigned char ca_certs[] = R"(-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)";
#define sizeof_ca_certs sizeof(ca_certs)

// static const unsigned char priv_key[] = R"(
// )";
// #define sizeof_priv_key sizeof(priv_key)

// static const unsigned char cert[] = R"(
// )";
// #define sizeof_cert sizeof(cert)

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
    ret->err = "returned without setting error";

    int intret;

    if(wolfSSL_Debugging_ON()) {
        ret->err = "logging on, before init";
        return;
    }

    // ssl initialization.
    if(wolfSSL_Init() != SSL_SUCCESS)
    {
        ret->err = "wolfSSL_Init() error";
        return;
    }

    if(wolfSSL_Debugging_ON()) {
        ret->err = "logging on, after init";
        return;
    }

    //init context
    WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    if (!ctx)
    {
        ret->err = "cannot create SSL context";
        return;
    }

    /* Load client certificates into WOLFSSL_CTX */
    if ((intret = wolfSSL_CTX_load_verify_buffer(ctx, ca_certs,
            sizeof_ca_certs - 1, SSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
        ret->err = "failed to load CA certs";
        return;
    }

    // if ((intret = wolfSSL_CTX_use_PrivateKey_buffer(ctx,
    //         priv_key,
    //         sizeof_priv_key,
    //         WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
    //     ret->err = "failed to load private key";
    //     return;
    // }

    // if ((intret = wolfSSL_CTX_use_certificate_buffer(ctx,
    //         cert,
    //         sizeof_cert,
    //         WOLFSSL_FILETYPE_PEM)) != WOLFSSL_SUCCESS) {
    //     ret->err = "failed to load certificate";
    //     return;
    // }

    if (SSL_SUCCESS != wolfSSL_CTX_UseSNI(ctx, WOLFSSL_SNI_HOST_NAME, domain, strlen(domain)))
    {
        ret->err = "wolfSSL_CTX_UseSNI() failed";
        return;
    }

    // wolfSSL_CTX_set_verify(ctx, SSL_VERIFY_NONE, 0);

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

    // while(peer) {
    //     char peer_addr[50];
    //     char peer_protocol[50];
    //     if(0 != getnameinfo(peer->ai_addr, peer->ai_addrlen, peer_addr, sizeof(peer_addr), peer_protocol, sizeof(peer_protocol), NI_NUMERICHOST))
    //     {
    //         ret->err = "getnameinfo() error";
    //         return;
    //     }

    //     printf("%s\n", peer_addr);

    //     peer = peer->ai_next;
    // }
    
    // return;

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
    ret->err = NULL;
}

void xkcd_app(void)
{
    // pngle_t * dec = pngle_new();
    // assert(dec);

    // pstate_t * ps = malloc(sizeof(pstate_t));
    // assert(ps);
    // ps->canv = lv_canvas_create(lv_screen_active());
    // ps->done = false;

    // pngle_set_user_data(dec, ps);
    // pngle_set_init_callback(dec, init_cb);
    // pngle_set_draw_callback(dec, draw_cb);
    // pngle_set_done_callback(dec, done_cb);

    // // for(size_t i = 0; i < sizeof(comic_bin) && !ps->done; i += 1024) {
    // //     int ret = pngle_feed(dec, comic_bin + i, 1024);
    // //     assert(ret == 1024);
    // // }

    // assert(!ps->done);
    // int ret = pngle_feed(dec, comic_bin, sizeof(comic_bin));
    // assert(ret == sizeof(comic_bin));
    // assert(ps->done);

    // pngle_destroy(dec);
    // // void * buf = lv_canvas_get_buf(ps->canv);
    // // lv_obj_delete(ps->canv);
    // // free(buf);
    // free(ps);

    ///////////////////////////////////////////////

    https_connect_ret_t conn_ret;
    https_connect(&conn_ret, HOST);
    printf("con ret err: %s\n", conn_ret.err ? conn_ret.err : "no error");

    if (!conn_ret.err) {
        static const char data[] = "GET  /_matrix/client/v3/login HTTP/1.1\r\nHost: " HOST "\r\n\r\n";
        int intret = wolfSSL_write(conn_ret.ssl, data, sizeof(data) - 1);
        if(intret != sizeof(data) - 1) {
            printf("write failed");
            return;
        }
        char buf[2000] = {0};
        intret = wolfSSL_read(conn_ret.ssl, buf, sizeof(buf) - 1);
        puts(buf);

        // WOLFSSL_X509_CHAIN * chain = wolfSSL_get_peer_chain(conn_ret.ssl);
        // if(!chain){
        //     puts("chain issue");
        //     return;
        // }

        // int chain_len = wolfSSL_get_chain_count(chain);
        // printf("chain len: %d\n", chain_len);

        // for(int i = 0; i < chain_len; i++) {
        //     unsigned char buf[2000];
        //     int outlen = -1;
        //     wolfSSL_get_chain_cert_pem(chain, i, buf, 2000, &outlen);
        //     if(outlen == -1) {
        //         puts("get chain pem issue");
        //         return;
        //     }
        //     printf("outlen: %d\n%.*s\n", outlen, outlen, buf);
        // }
    }
}
