#pragma once

#define MCP_DECLARATION_LEN 32
#define MCP_MAX_REQUESTS 12

typedef struct {
    uint8_t location;
    bool is_horizontal;
    char declaration[MCP_DECLARATION_LEN];
} mcp_request_s;

typedef enum {
    MCP_ROUTE_E_SET,
    MCP_ROUTE_E_CLEAR,
    MCP_ROUTE_E_ROUTE
} mcp_route_e;

extern mcp_request_s mcp_g_requests[MCP_MAX_REQUESTS];

void mcp_declare(uint8_t idx, const char * declaration, uint8_t *location, bool *is_horizontal);
int mcp_request(uint8_t idx);
void mcp_route(uint8_t idx, mcp_route_e op, uint8_t in_mod, uint8_t in_pin, uint8_t out_mod, uint8_t out_pin);
void mcp_memory(uint8_t idx, bool is_read, uint8_t mod_n, uint8_t offset, uint8_t length, uint8_t *srcdst);
