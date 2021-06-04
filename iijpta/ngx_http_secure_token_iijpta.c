#include <ngx_config.h>

#include "ngx_http_secure_token_iijpta.h"
#include "../ngx_http_secure_token_filter_module.h"
#include "../ngx_http_secure_token_utils.h"

#include <openssl/evp.h>

// macros
#define set_be32(p, dw)						\
	{										\
	((u_char*)p)[0] = ((dw) >> 24) & 0xFF;	\
	((u_char*)p)[1] = ((dw) >> 16) & 0xFF;	\
	((u_char*)p)[2] = ((dw) >> 8) & 0xFF;	\
	((u_char*)p)[3] = (dw)& 0xFF;			\
	}

#define set_be64(p, qw)						\
	{										\
	set_be32(p, (qw) >> 32);				\
	set_be32(p + 4, (qw));					\
	}

// constants
#define CRC32_SIZE    4
#define EXPIRY_SIZE   8
#define PATH_LIMIT    1024
#define COOKIE_ATTR_SIZE (sizeof("; Expires=Thu, 31-Dec-2019 23:59:59 GMT; Max-Age=") - 1 + NGX_TIME_T_LEN)

// typedefs
typedef struct {
	ngx_str_t key;
	ngx_str_t iv;
	ngx_http_complex_value_t *acl;
	size_t acl_path_based;
	ngx_secure_token_time_t end;
} ngx_secure_token_iijpta_token_t;

typedef struct {
	u_char crc[CRC32_SIZE];
	u_char expiry[EXPIRY_SIZE];
} ngx_http_secure_token_iijpta_header_t;

static ngx_conf_num_bounds_t ngx_http_secure_token_iijpta_key_bounds = {
	ngx_conf_check_str_len_bounds, 16, 16
};

static ngx_conf_num_bounds_t ngx_http_secure_token_iijpta_iv_bounds = {
	ngx_conf_check_str_len_bounds, 16, 16
};

// globals
static ngx_command_t ngx_http_secure_token_iijpta_cmds[] = {
	{ ngx_string("key"),
	NGX_CONF_TAKE1,
	ngx_http_secure_token_conf_set_hex_str_slot,
	0,
	offsetof(ngx_secure_token_iijpta_token_t, key),
	&ngx_http_secure_token_iijpta_key_bounds },

	{ ngx_string("iv"),
	NGX_CONF_TAKE1,
	ngx_http_secure_token_conf_set_hex_str_slot,
	0,
	offsetof(ngx_secure_token_iijpta_token_t, iv),
	&ngx_http_secure_token_iijpta_iv_bounds },

	{ ngx_string("acl"),
	NGX_CONF_TAKE1,
	ngx_http_set_complex_value_slot,
	0,
	offsetof(ngx_secure_token_iijpta_token_t, acl),
	NULL },

	{ ngx_string("acl_path_based"),
	NGX_CONF_TAKE1,
	ngx_conf_set_size_slot,
	0,
	offsetof(ngx_secure_token_iijpta_token_t, acl_path_based),
	NULL },

	{ ngx_string("end"),
	NGX_CONF_TAKE1,
	ngx_http_secure_token_conf_set_time_slot,
	0,
	offsetof(ngx_secure_token_iijpta_token_t, end),
	NULL },
};

static ngx_int_t
ngx_http_secure_token_get_acl_iijpta(ngx_http_request_t *r, ngx_secure_token_iijpta_token_t *token, ngx_str_t *acl)
{
	ngx_http_secure_token_loc_conf_t *conf;
	ngx_http_complex_value_t *acl_conf = token->acl;

	conf = ngx_http_get_module_loc_conf(r, ngx_http_secure_token_filter_module);

	// get the acl
	if (token->acl_path_based != NGX_CONF_UNSET_SIZE) {
		if (token->acl_path_based == 0) {
			ngx_str_set(acl, "/*");
			return NGX_OK;
		} else {
			size_t cnt = 0;
			size_t idx = r->uri.len - 1;
			while (idx > 0)
			{
				if (r->uri.data[idx] == '/') {
					cnt++;
				}
				if (cnt == token->acl_path_based) {
					u_char *p;
					size_t len = &r->uri.data[idx] - r->uri.data;
					p = ngx_pnalloc(r->pool, len + sizeof("/*") - 1);
					acl->data = p;
					acl->len  = len + sizeof("/*") - 1;
					p = ngx_copy(p, r->uri.data, len);
					p = ngx_copy(p, "/*", sizeof("/*") - 1);
					return NGX_OK;
				}
				idx--;
			}
		}
		ngx_str_set(acl, "/*");
		return NGX_OK;
	} else {
		if (acl_conf != NULL)
		{
			if (ngx_http_complex_value(r, acl_conf, acl) != NGX_OK)
			{
				return NGX_ERROR;
			}
		}
		else
		{
			// the default is '/*'
			ngx_str_set(acl, "/*");
		}
	}

	return NGX_OK;
}

static ngx_int_t
ngx_secure_token_iijpta_get_var(
	ngx_http_request_t *r,
	ngx_http_variable_value_t *v,
	uintptr_t data)
{
	EVP_CIPHER_CTX *ctx = NULL;
	uint32_t crc;
	ngx_secure_token_iijpta_token_t* token = (void*)data;
	ngx_http_secure_token_iijpta_header_t hdr;
	ngx_http_secure_token_loc_conf_t *conf;
	size_t in_len;
	size_t size;
	u_char *p;
	u_char *out;
	int out_len;
	u_char *outp;
	uint64_t end;
	ngx_str_t acl;
	ngx_int_t rc;

	conf = ngx_http_get_module_loc_conf(r, ngx_http_secure_token_filter_module);

	rc = ngx_http_secure_token_get_acl_iijpta(r, token, &acl);
	if (rc != NGX_OK)
	{
		return rc;
	}

	if (acl.len > PATH_LIMIT)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: acl is too long for the iijpta token");
		return NGX_ERROR;
	}

	end = token->end.val;
	if (token->end.type == NGX_HTTP_SECURE_TOKEN_TIME_RELATIVE)
	{
		end += ngx_time();
	}
	set_be64(hdr.expiry, end);

	ngx_crc32_init(crc);
	ngx_crc32_update(&crc, hdr.expiry, EXPIRY_SIZE);
	ngx_crc32_update(&crc, acl.data, acl.len);
	ngx_crc32_final(crc);
	set_be32(hdr.crc, crc);

	ctx = EVP_CIPHER_CTX_new();
	if (ctx == NULL)
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: EVP_CIPHER_CTX_new failed");
		return NGX_ERROR;
	}

	if (!EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, token->key.data, token->iv.data))
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: EVP_EncryptInit_ex failed");
		goto error;
	}

	in_len = sizeof(ngx_http_secure_token_iijpta_header_t) + acl.len;
	// in_len rounded up to block + one block for padding
	out = ngx_pnalloc(r->pool, (in_len & ~0xf) + 0x10);
	if (out == NULL)
	{
		goto error;
	}

	outp = out;
	if (!EVP_EncryptUpdate(ctx, outp, &out_len, (void *)&hdr, sizeof(hdr)))
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: EVP_EncryptUpdate failed (1)");
		goto error;
	}
	outp += out_len;

	if (!EVP_EncryptUpdate(ctx, outp, &out_len, acl.data, acl.len))
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: EVP_EncryptUpdate failed (2)");
		goto error;
	}
	outp += out_len;

	if (!EVP_EncryptFinal_ex(ctx, outp, &out_len))
	{
		ngx_log_error(NGX_LOG_ERR, r->connection->log, 0,
			      "ngx_secure_token_iijpta_get_var: EVP_EncryptFinal_ex failed");
		goto error;
	}
	outp += out_len;
	out_len = outp - out;

	size = sizeof("pta=") + (out_len * 2);
	if (conf->avoid_cookies == 0)
	{
	        size += COOKIE_ATTR_SIZE;
	}

	p = ngx_pnalloc(r->pool, size);
	if (p == NULL)
	{
	        goto error;
	}
	v->data = p;
	p = ngx_copy(p, "pta=", sizeof("pta=") - 1);
	p = ngx_hex_dump(p, out, out_len);

	if (conf->avoid_cookies == 0)
	{
	        p = ngx_copy(p, "; Expires=", sizeof("; Expires=")  - 1);
		p = ngx_http_cookie_time(p, end);
		p = ngx_sprintf(p, "; Max-Age=%T", end - ngx_time());
	}

	*p = '\0';

	v->len = p - v->data;
	v->valid = 1;
	v->no_cacheable = 0;
	v->not_found = 0;

	EVP_CIPHER_CTX_free(ctx);

	return NGX_OK;

error:
	EVP_CIPHER_CTX_free(ctx);
	return NGX_ERROR;
}

char *
ngx_secure_token_iijpta_block(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
	ngx_secure_token_iijpta_token_t* token;
	char* rv;

	// init config
	token = ngx_pcalloc(cf->pool, sizeof(*token));
	if (token == NULL)
	{
		return NGX_CONF_ERROR;
	}

	token->end.type = NGX_HTTP_SECURE_TOKEN_TIME_UNSET;
	token->acl_path_based = NGX_CONF_UNSET_SIZE;

	// parse the block
	rv = ngx_http_secure_token_conf_block(
		cf,
		ngx_http_secure_token_iijpta_cmds,
		token,
		ngx_secure_token_iijpta_get_var);
	if (rv != NGX_CONF_OK)
	{
		return rv;
	}

	if (token->end.type == NGX_HTTP_SECURE_TOKEN_TIME_UNSET)
	{
		token->end.type = NGX_HTTP_SECURE_TOKEN_TIME_RELATIVE;
		token->end.val = 86400;
	}

	return NGX_CONF_OK;
}
