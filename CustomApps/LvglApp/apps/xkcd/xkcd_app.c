#include "../../apps.h"

#include "lvgl/lvgl.h"
#include "netutils/cJSON.h"

#include "pngle/pngle.h"

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
#include <stddef.h>

#ifdef NDEBUG
  #error must be refactored to support NDEBUG. i.e. change `assert(foo())` to `x=foo();assert(x)`
#endif

typedef struct {
    void * decoded;
    lv_obj_t * canv;
    bool done;
} pstate_t;

static const unsigned char ca_certs[] = R"(-----BEGIN CERTIFICATE-----
MIIDXzCCAkegAwIBAgILBAAAAAABIVhTCKIwDQYJKoZIhvcNAQELBQAwTDEgMB4G
A1UECxMXR2xvYmFsU2lnbiBSb290IENBIC0gUjMxEzARBgNVBAoTCkdsb2JhbFNp
Z24xEzARBgNVBAMTCkdsb2JhbFNpZ24wHhcNMDkwMzE4MTAwMDAwWhcNMjkwMzE4
MTAwMDAwWjBMMSAwHgYDVQQLExdHbG9iYWxTaWduIFJvb3QgQ0EgLSBSMzETMBEG
A1UEChMKR2xvYmFsU2lnbjETMBEGA1UEAxMKR2xvYmFsU2lnbjCCASIwDQYJKoZI
hvcNAQEBBQADggEPADCCAQoCggEBAMwldpB5BngiFvXAg7aEyiie/QV2EcWtiHL8
RgJDx7KKnQRfJMsuS+FggkbhUqsMgUdwbN1k0ev1LKMPgj0MK66X17YUhhB5uzsT
gHeMCOFJ0mpiLx9e+pZo34knlTifBtc+ycsmWQ1z3rDI6SYOgxXG71uL0gRgykmm
KPZpO/bLyCiR5Z2KYVc3rHQU3HTgOu5yLy6c+9C7v/U9AOEGM+iCK65TpjoWc4zd
QQ4gOsC0p6Hpsk+QLjJg6VfLuQSSaGjlOCZgdbKfd/+RFO+uIEn8rUAVSNECMWEZ
XriX7613t2Saer9fwRPvm2L7DWzgVGkWqQPabumDk3F2xmmFghcCAwEAAaNCMEAw
DgYDVR0PAQH/BAQDAgEGMA8GA1UdEwEB/wQFMAMBAf8wHQYDVR0OBBYEFI/wS3+o
LkUkrk1Q+mOai97i3Ru8MA0GCSqGSIb3DQEBCwUAA4IBAQBLQNvAUKr+yAzv95ZU
RUm7lgAJQayzE4aGKAczymvmdLm6AC2upArT9fHxD4q/c2dKg8dEe3jgr25sbwMp
jjM5RcOO5LlXbKr8EpbsU8Yt5CRsuZRj+9xTaGdWPoO4zzUhw8lo/s7awlOqzJCK
6fBdRoyV3XpYKBovHd7NADdBj+1EbddTKJd+82cEHhXXipa0095MJ6RMG3NzdvQX
mcIfeg7jLQitChws/zyrVQ4PkX4268NXSb7hLi18YIvDQVETI53O9zJrlAGomecs
Mx86OyXShkDOOyyGeMlhLxS67ttVb9+E7gUJTb0o2HLO02JQZR7rkpeDMdmztcpH
WD9f
-----END CERTIFICATE-----
)";
#define CA_CERTS_LEN (sizeof(ca_certs) - 1)

#define BUF_SZ 1000
#define OK_RESP "HTTP/1.1 200 OK\r\n"
#define OK_RESP_LEN (sizeof(OK_RESP) - 1)

static void init_cb(pngle_t *dec, uint32_t w, uint32_t h) {
    pstate_t * ps = pngle_get_user_data(dec);
    ps->decoded = malloc(w * h * 2);
    assert(ps->decoded);
    lv_canvas_set_buffer(ps->canv, ps->decoded, w, h, LV_COLOR_FORMAT_RGB565);
}

static void draw_cb(pngle_t *dec, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]) {
    (void)w;
    (void)h;
    pstate_t * ps = pngle_get_user_data(dec);
    lv_canvas_set_px(ps->canv, x, y, lv_color_make(rgba[0], rgba[1], rgba[2]), rgba[3]);
}

static void done_cb(pngle_t *dec) {
    pstate_t * ps = pngle_get_user_data(dec);
    ps->done = true;
}

static void https_init(void)
{
    // assert(!wolfSSL_Debugging_ON());
    assert(wolfSSL_Init() == SSL_SUCCESS);
    // assert(!wolfSSL_Debugging_ON());
}

static WOLFSSL_CTX * https_ctx(void)
{
    WOLFSSL_CTX *ctx = wolfSSL_CTX_new(wolfTLSv1_2_client_method());
    assert(ctx);

    assert(wolfSSL_CTX_load_verify_buffer(ctx, ca_certs,
           CA_CERTS_LEN, SSL_FILETYPE_PEM) == WOLFSSL_SUCCESS);

    return ctx;
}

static WOLFSSL * https_connect(WOLFSSL_CTX * ctx, const char * domain) {
    /* code taken from tcp_client_classic.c */
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    struct addrinfo *peer;
    hints.ai_socktype = SOCK_STREAM;
    assert(0 == getaddrinfo(domain, "443", &hints, &peer));

    int fd = socket(peer->ai_family, peer->ai_socktype, peer->ai_protocol);
    assert(fd >= 0);

    assert(0 == connect(fd, peer->ai_addr, peer->ai_addrlen));
    /* END */
    freeaddrinfo(peer);
    peer = NULL;

    WOLFSSL *ssl = wolfSSL_new(ctx);
    assert(ssl);

    assert(SSL_SUCCESS == wolfSSL_UseSNI(ssl, WOLFSSL_SNI_HOST_NAME, domain, strlen(domain)));

    assert(wolfSSL_set_fd(ssl, fd) == SSL_SUCCESS);

    assert(wolfSSL_connect(ssl) == SSL_SUCCESS);

    return ssl;
}

typedef struct {
    char * headers;
    size_t content_len;
    uint8_t * content;
    size_t content_buffered;
} https_get_t;

typedef struct {
    WOLFSSL * ssl;
    char * buf;
    size_t cap;
    size_t len;
} https_get_more_t;

static void https_get_more(https_get_more_t * more, size_t total_goal)
{
    if(!total_goal) {
        total_goal = more->cap;
        if(total_goal == more->len) {
            total_goal += BUF_SZ;
        }
    }

    while (more->len < total_goal) {
        if(more->len == more->cap) {
            more->cap += BUF_SZ;
            more->buf = realloc(more->buf, more->cap);
            assert(more->buf);
        }

        int n_read = wolfSSL_read(more->ssl, &more->buf[more->len], more->cap - more->len);
        assert(n_read > 0);
        more->len += n_read;
    }
}

static void https_get(WOLFSSL * ssl, https_get_t * get, const char * domain_reminder, const char * path)
{
    char * req;
    int req_len = asprintf(&req, "GET /%s HTTP/1.1\r\nHost: %s\r\n\r\n",
                           path, domain_reminder);
    assert(req_len != -1);

    int n_written = wolfSSL_write(ssl, req, req_len);
    assert(n_written == req_len);

    free(req);
    req = NULL;

    https_get_more_t more = {
        .ssl = ssl,
        .buf = malloc(BUF_SZ),
        .cap = BUF_SZ,
        .len = 0
    };
    assert(more.buf);

    https_get_more(&more, OK_RESP_LEN);
    assert(0 == memcmp(OK_RESP, more.buf, OK_RESP_LEN));

    size_t i = OK_RESP_LEN;
    char * end;
    while(NULL == (end = memmem(&more.buf[i], more.len - i, "\r\n\r\n", 4))) {
        https_get_more(&more, 0);
    }
    size_t headers_len = end - more.buf;
    end[2] = '\0';

    char * cur = &more.buf[i - 2];
    assert(NULL != strcasestr(cur, "\r\nconnection: keep-alive\r\n"));
    const char * header = "\r\ncontent-length:";
    cur = strcasestr(cur, header);
    assert(cur);
    cur += strlen(header);
    unsigned int content_len;
    assert(1 == sscanf(cur, "%u", &content_len));

    end[2] = '\r';

    get->headers = more.buf;
    get->content_len = content_len;
    get->content = (uint8_t *) end + 4;
    get->content_buffered = more.len - headers_len - 4;
}

static void ssl_read_exactly(WOLFSSL * ssl, uint8_t * dst, size_t size)
{
    while(size) {
        int n_read = wolfSSL_read(ssl, dst, size);
        assert(n_read > 0);
        size -= n_read;
        dst += n_read;
    }
}

typedef struct {
    WOLFSSL * json_ssl;
    WOLFSSL * img_ssl;
    unsigned comic_on;
    unsigned comic_max;
    void * decoded;
} load_comic_t;

static void load_comic(load_comic_t * comic_ctx);

static void left_btn_cb(lv_event_t * e)
{
    load_comic_t * comic_ctx = lv_event_get_user_data(e);
    comic_ctx->comic_on--;
    load_comic(comic_ctx);
}

static void right_btn_cb(lv_event_t * e)
{
    load_comic_t * comic_ctx = lv_event_get_user_data(e);
    comic_ctx->comic_on++;
    load_comic(comic_ctx);
}

static void load_comic(load_comic_t * comic_ctx)
{
    char * path;
    void * free_me = NULL;
    if (comic_ctx->comic_on > 0) {
        assert(-1 != asprintf(&path, "%u/info.0.json", comic_ctx->comic_on));
        free_me = path;
    } else {
        path = "info.0.json";
    }

    https_get_t get;
    https_get(comic_ctx->json_ssl, &get, "xkcd.com", path);
    free(free_me);

    char * json = malloc(get.content_len + 1);
    assert(json);
    memcpy(json, get.content, get.content_buffered);
    free(get.headers);
    ssl_read_exactly(comic_ctx->json_ssl, (uint8_t *) json + get.content_buffered, get.content_len - get.content_buffered);
    json[get.content_len] = '\0';
    // printf("%s\n", json);

    cJSON * cjson = cJSON_ParseWithOpts(json, NULL, 1);
    assert(cjson);
    free(json);

    cJSON * item;
    assert(cJSON_IsString((item = cJSON_GetObjectItemCaseSensitive(cjson, "img"))));
    const char * img_url = item->valuestring;
    assert(cJSON_IsString((item = cJSON_GetObjectItemCaseSensitive(cjson, "safe_title"))));
    const char * title = item->valuestring;
    assert(cJSON_IsString((item = cJSON_GetObjectItemCaseSensitive(cjson, "alt"))));
    const char * alt = item->valuestring;
    assert(cJSON_IsNumber((item = cJSON_GetObjectItemCaseSensitive(cjson, "num"))));
    int num = item->valueint;

    if(comic_ctx->comic_on <= 0) {
        comic_ctx->comic_on = num;
    } else {
        assert(num == comic_ctx->comic_on);
    }
    if(comic_ctx->comic_max <= 0) {
        comic_ctx->comic_max = num;
    }

    lv_obj_t * scr = lv_screen_active();
    lv_obj_clean(scr);
    free(comic_ctx->decoded);
    lv_obj_set_style_pad_all(scr, 8, 0);
    lv_group_add_obj(lv_group_get_default(), scr);
    lv_gridnav_add(scr, LV_GRIDNAV_CTRL_SCROLL_FIRST);
    lv_obj_set_layout(scr, LV_LAYOUT_GRID);
    static const int32_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const int32_t row_dsc[] = {LV_GRID_CONTENT,
                                      LV_GRID_CONTENT,
                                      LV_GRID_FR(1),
                                      LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(scr, col_dsc, row_dsc);

    lv_obj_t * title_label = lv_label_create(scr);
    // lv_obj_set_width(title_label, LV_PCT(100));
    lv_label_set_text(title_label, title);
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(title_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_grid_cell(title_label, LV_GRID_ALIGN_STRETCH, 0, 4, LV_GRID_ALIGN_START, 0, 1);

    lv_obj_t * left_btn = lv_button_create(scr);
    lv_obj_add_event_cb(left_btn, left_btn_cb, LV_EVENT_CLICKED, comic_ctx);
    lv_group_remove_obj(left_btn);
    lv_obj_set_grid_cell(left_btn, LV_GRID_ALIGN_STRETCH, 1, 1, LV_GRID_ALIGN_START, 1, 1);
    lv_obj_t * left_btn_label = lv_label_create(left_btn);
    lv_obj_center(left_btn_label);
    lv_label_set_text_static(left_btn_label, LV_SYMBOL_LEFT);

    lv_obj_t * right_btn = lv_button_create(scr);
    if(num == comic_ctx->comic_max) {
        lv_obj_add_state(right_btn, LV_STATE_DISABLED);
    } else {
        lv_obj_add_event_cb(right_btn, right_btn_cb, LV_EVENT_CLICKED, comic_ctx);
    }
    lv_group_remove_obj(right_btn);
    lv_obj_set_grid_cell(right_btn, LV_GRID_ALIGN_STRETCH, 2, 1, LV_GRID_ALIGN_START, 1, 1);
    lv_obj_t * right_btn_label = lv_label_create(right_btn);
    lv_obj_center(right_btn_label);
    lv_label_set_text_static(right_btn_label, LV_SYMBOL_RIGHT);

    lv_obj_t * canv_cont = lv_obj_create(scr);
    lv_obj_set_style_bg_color(canv_cont, lv_palette_lighten(LV_PALETTE_BLUE, 5), LV_STATE_FOCUSED);
    lv_obj_set_grid_cell(canv_cont, LV_GRID_ALIGN_STRETCH, 0, 4, LV_GRID_ALIGN_STRETCH, 2, 1);

    lv_obj_t * canv = lv_canvas_create(canv_cont);
    lv_group_remove_obj(canv);
    // lv_obj_align(canv, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t * num_label = lv_label_create(scr);
    lv_obj_set_style_text_font(num_label, &lv_font_montserrat_12, 0);
    lv_label_set_text_fmt(num_label, "#%d", num);
    lv_obj_set_grid_cell(num_label, LV_GRID_ALIGN_START, 0, 1, LV_GRID_ALIGN_END, 1, 1);


    // lv_obj_set_flex_flow(scr, LV_FLEX_FLOW_COLUMN);
    // lv_obj_set_flex_align(scr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    // lv_obj_set_style_pad_top(scr, 16, 0);
    // lv_obj_set_style_pad_bottom(scr, 16, 0);
    // lv_obj_t * title_label = lv_label_create(scr);
    // scroll_helper(title_label, LV_GRIDNAV_CTRL_NONE);
    // lv_obj_set_width(title_label, LV_PCT(95));
    // lv_label_set_text(title_label, title);
    // lv_obj_set_style_text_font(title_label, &lv_font_montserrat_26, 0);
    // lv_obj_t * btn_cont = lv_obj_create(scr);
    // lv_obj_set_style_border_width(btn_cont, 0, 0);
    // lv_obj_set_style_pad_all(btn_cont, 5, 0);
    // lv_obj_set_style_bg_opa(btn_cont, LV_OPA_TRANSP, 0);
    // lv_obj_set_size(btn_cont, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    // lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW);
    // lv_obj_t * left_btn = lv_button_create(btn_cont);
    // scroll_helper(left_btn, LV_GRIDNAV_CTRL_NONE);
    // lv_label_set_text_static(lv_label_create(left_btn), LV_SYMBOL_LEFT);
    // lv_obj_add_event_cb(left_btn, left_btn_cb, LV_EVENT_CLICKED, comic_ctx);
    // lv_obj_t * right_btn = lv_button_create(btn_cont);
    // scroll_helper(right_btn, LV_GRIDNAV_CTRL_NONE);
    // lv_label_set_text_static(lv_label_create(right_btn), LV_SYMBOL_RIGHT);
    // if(num == comic_ctx->comic_max) {
    //     lv_obj_add_state(right_btn, LV_STATE_DISABLED);
    // } else {
    //     lv_obj_add_event_cb(right_btn, right_btn_cb, LV_EVENT_CLICKED, comic_ctx);
    // }
    // lv_obj_t * canv_cont = lv_obj_create(scr);
    // scroll_helper(canv_cont, LV_GRIDNAV_CTRL_SCROLL_FIRST);
    // lv_obj_set_size(canv_cont, LV_PCT(100), LV_SIZE_CONTENT);
    // lv_obj_t * canv = lv_canvas_create(canv_cont);
    // lv_obj_t * alt_label = lv_label_create(scr);
    // scroll_helper(alt_label, LV_GRIDNAV_CTRL_NONE);
    // lv_label_set_text(alt_label, alt);
    // lv_obj_set_width(alt_label, LV_PCT(95));
    // lv_label_set_text_fmt(lv_label_create(scr), "#%d", num);

    const char * img_url_expected_start = "https://imgs.xkcd.com/";
    size_t img_url_expected_start_len = strlen(img_url_expected_start);
    assert(0 == memcmp(img_url, img_url_expected_start, img_url_expected_start_len));
    size_t img_url_len = strlen(img_url);
    assert(img_url_len >= 4 && 0 == strcmp(img_url + (img_url_len - 4), ".png"));
    const char * img_path = img_url + img_url_expected_start_len;
    https_get(comic_ctx->img_ssl, &get, "imgs.xkcd.com", img_path);

    cJSON_Delete(cjson);

    uint8_t * raw_png = malloc(get.content_len);
    assert(raw_png);

    memcpy(raw_png, get.content, get.content_buffered);
    free(get.headers);
    ssl_read_exactly(comic_ctx->img_ssl, raw_png + get.content_buffered, get.content_len - get.content_buffered);

    pstate_t ps = {
        .decoded = NULL,
        .canv = canv,
        .done = false
    };
    pngle_t * dec = pngle_new();
    assert(dec);
    pngle_set_user_data(dec, &ps);
    pngle_set_init_callback(dec, init_cb);
    pngle_set_draw_callback(dec, draw_cb);
    pngle_set_done_callback(dec, done_cb);

    assert(!ps.done);
    int n_fed = pngle_feed(dec, raw_png, get.content_len);
    assert(n_fed == get.content_len);
    assert(ps.done);

    pngle_destroy(dec);

    assert(ps.decoded);
    comic_ctx->decoded = ps.decoded;

    free(raw_png);
}

void xkcd_app(void)
{
    https_init();
    WOLFSSL_CTX * ctx = https_ctx();

    static load_comic_t comic_ctx;

    comic_ctx.json_ssl = https_connect(ctx, "xkcd.com");
    comic_ctx.img_ssl = https_connect(ctx, "imgs.xkcd.com");
    comic_ctx.comic_on = 0;
    comic_ctx.comic_max = 0;
    comic_ctx.decoded = NULL;

    load_comic(&comic_ctx);
}
