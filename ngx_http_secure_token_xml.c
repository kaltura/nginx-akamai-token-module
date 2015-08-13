#include "ngx_http_secure_token_xml.h"

enum {
	STATE_TAG_NAME = STATE_URL_LAST,
	STATE_CLOSING_TAG_NAME,
	STATE_ATTR_NAME,
	STATE_ATTR_VALUE,
	STATE_ATTR_QUOTED_VALUE,
};

static ngx_flag_t
ngx_http_secure_token_xml_is_relevant_attr(
	ngx_http_secure_token_xml_ctx_t* ctx,
	ngx_http_secure_token_xml_node_attrs_t* nodes)
{
	ngx_http_secure_token_xml_node_attrs_t* cur_node;
	ngx_str_t* cur_attr;

	for (cur_node = nodes; cur_node->tag_name.len != 0; cur_node++)
	{
		if (ctx->tag_name_len != cur_node->tag_name.len ||
			ngx_strncasecmp(ctx->tag_name, cur_node->tag_name.data, cur_node->tag_name.len) != 0)
		{
			continue;
		}

		for (cur_attr = cur_node->attr_names; cur_attr->len != 0; cur_attr++)
		{
			if (ctx->attr_name_len != cur_attr->len ||
				ngx_strncasecmp(ctx->attr_name, cur_attr->data, cur_attr->len) != 0)
			{
				continue;
			}

			return 1;
		}
	}

	return 0;
}

ngx_chain_t** 
ngx_http_secure_token_xml_processor(
	ngx_http_request_t* r,
	ngx_http_secure_token_processor_conf_t* conf,
	ngx_http_secure_token_xml_node_attrs_t* nodes,
	ngx_buf_t *in, 
	ngx_http_secure_token_ctx_t* root_ctx,
	ngx_http_secure_token_xml_ctx_t* ctx, 
	ngx_chain_t** out)
{
	u_char* last_sent;
	u_char* cur_pos;
	u_char* buffer_end;
	u_char ch;

	last_sent = in->pos;
	buffer_end = in->last;
	for (cur_pos = in->pos; cur_pos < buffer_end; cur_pos++)
	{
		ch = *cur_pos;

		switch (ctx->base.state)
		{
		case STATE_INITIAL:
			if (ch == '<')
			{
				ctx->base.state = STATE_TAG_NAME;
				ctx->tag_name_len = 0;
			}
			break;

		case STATE_TAG_NAME:
		case STATE_CLOSING_TAG_NAME:
			if (isspace(ch))
			{
				if (ctx->tag_name_len == 0)
				{
					break;
				}
				ctx->base.state = STATE_ATTR_NAME;
				ctx->attr_name_len = 0;
			}
			else if (ch == '>')
			{
				ctx->base.state = STATE_INITIAL;
			}
			else if (ch == '/' && ctx->tag_name_len == 0)
			{
				ctx->base.state = STATE_CLOSING_TAG_NAME;
			}
			else if (ctx->tag_name_len < XML_MAX_TAG_NAME_LEN)
			{
				ctx->tag_name[ctx->tag_name_len] = ch;
				ctx->tag_name_len++;
			}
			break;

		case STATE_ATTR_NAME:
			if (isspace(ch))
			{
				break;
			}
			if (ch == '=')
			{
				ctx->base.state = STATE_ATTR_VALUE;
			}
			else if (ch == '>')
			{
				ctx->base.state = STATE_INITIAL;
			}
			else if (ctx->attr_name_len < XML_MAX_ATTR_NAME_LEN)
			{
				ctx->attr_name[ctx->attr_name_len] = ch;
				ctx->attr_name_len++;
			}
			break;

		case STATE_ATTR_VALUE:
			if (ch == '"')
			{
				if (ngx_http_secure_token_xml_is_relevant_attr(ctx, nodes))
				{
					ctx->base.state = STATE_URL_SCHEME;
					ctx->base.scheme_pos = 0;
					ctx->base.tokenize = 1;
					ctx->base.url_end_state = STATE_ATTR_NAME;
					ctx->base.url_end_char = '"';
					break;
				}

				ctx->base.state = STATE_ATTR_QUOTED_VALUE;
			}
			break;

		case STATE_ATTR_QUOTED_VALUE:
			if (ch != '"')
			{
				break;
			}

			ctx->base.state = STATE_ATTR_NAME;
			ctx->attr_name_len = 0;
			break;

		default:
			out = ngx_http_secure_token_url_state_machine(
				r,
				conf,
				root_ctx,
				&ctx->base,
				buffer_end,
				&cur_pos,
				&last_sent,
				out);
			if (out == NULL)
			{
				return NULL;
			}
			break;
		}
	}

	if (cur_pos > last_sent)
	{
		out = ngx_http_secure_token_add_to_chain(r->pool, last_sent, cur_pos, 1, 0, out);
		if (out == NULL)
		{
			return NULL;
		}
	}

	return out;
}
