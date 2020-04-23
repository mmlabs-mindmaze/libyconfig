#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <strings.h>
#include <yaml.h>

#include "yconfig.h"

enum value_type {
	UNKNOWN,
	DATA,
	LIST,
	DICT,
};

struct yconfig_list {
	int length;
	int max_length;
	struct yconfig ** elts;
};

union value {
	char * s;
	struct yconfig_list l;
};

struct yconfig {
	struct yconfig * parent;
	char * key;
	enum value_type type;
	union value value;
};

static inline
int yconfig_is_root(struct yconfig const * conf)
{
	return (conf->parent == NULL);
}

static
char * _strndup(char const * ptr, size_t len)
{
	char * rv = malloc(len + 1);
	if (rv != NULL) {
		memcpy(rv, ptr, len);
		rv[len] = '\0';
	}
	return rv;
}

static
int yconfig_append(struct yconfig * parent, struct yconfig * conf)
{
	void * tmp;
	struct yconfig_list * list = &parent->value.l;
	int new_maw_length;
	int new_length = list->length + 1;

	assert(parent->type == LIST || parent->type == DICT);

	if (new_length >= list->max_length) {
		new_maw_length = !list->max_length ? 4 : list->max_length * 2;
		tmp = realloc(list->elts, new_maw_length * sizeof(*list->elts));
		if (tmp == NULL)
			return -1;

		list->elts = tmp;
		list->max_length = new_maw_length;
	}

	conf->parent = parent;
	list->elts[list->length] = conf;
	list->length = new_length;

	return 0;
}

static void value_destroy(enum value_type type, union value * value)
{
	int i;

	switch (type) {
		case UNKNOWN: /* FIXME */
		case DATA:
			free(value->s);
			break;
		case LIST: /* fallthrough */
		case DICT:
			for (i = 0 ; i < value->l.length ; i++)
				yconfig_destroy(value->l.elts[i]);

			free(value->l.elts);
			break;
		default:
			break;
	}
}

static inline
void dprint(int token_type)
{
	switch (token_type) {
		case YAML_NO_TOKEN: printf("YAML_NO_TOKEN\n"); break;
		case YAML_STREAM_START_TOKEN: printf("YAML_STREAM_START_TOKEN\n"); break;
		case YAML_STREAM_END_TOKEN: printf("YAML_STREAM_END_TOKEN\n"); break;
		case YAML_VERSION_DIRECTIVE_TOKEN: printf("YAML_VERSION_DIRECTIVE_TOKEN\n"); break;
		case YAML_TAG_DIRECTIVE_TOKEN: printf("YAML_TAG_DIRECTIVE_TOKEN\n"); break;
		case YAML_DOCUMENT_START_TOKEN: printf("YAML_DOCUMENT_START_TOKEN\n"); break;
		case YAML_DOCUMENT_END_TOKEN: printf("YAML_DOCUMENT_END_TOKEN\n"); break;
		case YAML_BLOCK_SEQUENCE_START_TOKEN: printf("YAML_BLOCK_SEQUENCE_START_TOKEN\n"); break;
		case YAML_BLOCK_MAPPING_START_TOKEN: printf("YAML_BLOCK_MAPPING_START_TOKEN\n"); break;
		case YAML_BLOCK_END_TOKEN: printf("YAML_BLOCK_END_TOKEN\n"); break;
		case YAML_FLOW_SEQUENCE_START_TOKEN: printf("YAML_FLOW_SEQUENCE_START_TOKEN\n"); break;
		case YAML_FLOW_SEQUENCE_END_TOKEN: printf("YAML_FLOW_SEQUENCE_END_TOKEN\n"); break;
		case YAML_FLOW_MAPPING_START_TOKEN: printf("YAML_FLOW_MAPPING_START_TOKEN\n"); break;
		case YAML_FLOW_MAPPING_END_TOKEN: printf("YAML_FLOW_MAPPING_END_TOKEN\n"); break;
		case YAML_BLOCK_ENTRY_TOKEN: printf("YAML_BLOCK_ENTRY_TOKEN\n"); break;
		case YAML_FLOW_ENTRY_TOKEN: printf("YAML_FLOW_ENTRY_TOKEN\n"); break;
		case YAML_KEY_TOKEN: printf("YAML_KEY_TOKEN\n"); break;
		case YAML_VALUE_TOKEN: printf("YAML_VALUE_TOKEN\n"); break;
		case YAML_ALIAS_TOKEN: printf("YAML_ALIAS_TOKEN\n"); break;
		case YAML_ANCHOR_TOKEN: printf("YAML_ANCHOR_TOKEN\n"); break;
		case YAML_TAG_TOKEN: printf("YAML_TAG_TOKEN\n"); break;
		case YAML_SCALAR_TOKEN: printf("YAML_SCALAR_TOKEN\n"); break;
		default:
			printf("ERR: unknown token : %d\n", token_type);
			break;
	}
}

/* TODO: get rid of all the assert() */
static int parse_config(struct yconfig * _conf, yaml_parser_t * parser)
{
	yaml_token_t token;
	struct yconfig * sub;
	char const * data;
	size_t datalen;
	int rv, type;
	char * key;
	struct yconfig * conf = _conf;

	assert(conf != NULL);
	memset(conf, 0, sizeof(*conf));

	rv = -1;
	type = -1;
	data = NULL;
	key = NULL;

	while (1) {
		if (!yaml_parser_scan(parser, &token)) {
			rv = 0;
			goto exit;
		}

		dprint(token.type);
		switch (token.type) {
		case YAML_NO_TOKEN:
			goto exit;

		case YAML_BLOCK_END_TOKEN:
		case YAML_STREAM_END_TOKEN:
			if (yconfig_is_root(conf)) {
				rv = 0;
				goto exit;
			}

			conf = conf->parent;
			printf("curr key: %s\n", conf->key ? conf->key : "root");
		break;

		case YAML_BLOCK_ENTRY_TOKEN:
			assert(type == YAML_VALUE_TOKEN || type == -1);
			conf->type = LIST;

			sub = yconfig_create();
			if (sub == NULL)
				goto exit;
			if (yconfig_append(conf, sub) != 0) {
				yconfig_destroy(sub);
				goto exit;
			}
			conf = sub;
			break;

		case YAML_FLOW_SEQUENCE_START_TOKEN:
		case YAML_BLOCK_SEQUENCE_START_TOKEN:
		case YAML_BLOCK_MAPPING_START_TOKEN:
		case YAML_FLOW_MAPPING_START_TOKEN:
			assert(type == YAML_VALUE_TOKEN || type == -1);
			conf->type = DICT;
			break;

		case YAML_FLOW_SEQUENCE_END_TOKEN:
		case YAML_FLOW_MAPPING_END_TOKEN:
			assert(conf && !yconfig_is_root(conf));
			conf = conf->parent;
			printf("curr key: %s\n", conf->key);
			break;

		case YAML_KEY_TOKEN:
			if (conf->type == DICT || conf->type == LIST) {
				sub = yconfig_create();
				if (sub == NULL)
					goto exit;
				if (yconfig_append(conf, sub) != 0) {
					yconfig_destroy(sub);
					goto exit;
				}
				conf = sub;
			}
			conf->type = DATA;

			type = YAML_KEY_TOKEN;
			break;

		case YAML_VALUE_TOKEN:
			type = YAML_VALUE_TOKEN;

			/* Will be changed to LIST/DICT if a MAPPING token is found */
			if (conf->type == UNKNOWN)
				conf->type = DATA;
			break;

		case YAML_SCALAR_TOKEN:
			data = (char const *) token.data.scalar.value;
			datalen = token.data.scalar.length;
			printf("parsing: \"%.*s\"\n", (int)datalen, data);

			if (type == YAML_KEY_TOKEN) {
				assert(conf->key == NULL);
				conf->key = _strndup(data, datalen);
				if (conf->key == NULL)
					goto exit;
			} else if (type == YAML_VALUE_TOKEN) {
				assert(conf->value.s == NULL);
				conf->value.s = _strndup(data, datalen);
				if (conf->value.s == NULL)
					goto exit;

				conf = conf->parent;
				printf("curr key: %s\n", conf->key);
			} else {
				assert(0);
			}

			break;
		default:          /* ignore */
			break;
		}

		yaml_token_delete(&token);
	}

exit:
	free(key);
	yaml_token_delete(&token);
	return rv;
}

struct yconfig * yconfig_create(void)
{
	struct yconfig * conf = malloc(sizeof(*conf));
	if (conf != NULL)
		memset(conf, 0, sizeof(*conf));

	return conf;
}

struct yconfig * yconfig_read(FILE * stream)
{
	int rv;
	struct yconfig * conf;
	yaml_parser_t parser;

	if (!yaml_parser_initialize(&parser))
		return NULL;

	yaml_parser_set_input_file(&parser, stream);

	conf = yconfig_create();
	if (conf == NULL)
		return NULL;

	rv = parse_config(conf, &parser);

	yaml_parser_delete(&parser);
	if (rv != 0) {
		yconfig_destroy(conf);
		return NULL;
	}

	return conf;
}

struct yconfig * yconfig_read_file(char const * filename)
{
	FILE * stream;
	struct yconfig * conf;

	stream = fopen(filename, "rb");
	if (stream == NULL)
		return NULL;

	conf = yconfig_read(stream);

	fclose(stream);
	return conf;
}

static inline void indent(FILE * stream, int n)
{
	for (int i = 0 ; i < n; i++)
		fprintf(stream, " ");
}

#define INDENT 2

static
int yconfig_write_rec(struct yconfig const * conf, FILE * stream, int n)
{
	int i;
	int rv;
	struct yconfig_list const * l;

	indent(stream, n);
	if (conf->key != NULL)
		fprintf(stream, "%s:%s", conf->key, conf->type == DATA ? " " : "\n");

	switch (conf->type) {
		case UNKNOWN: /* FIXME */
		case DATA:
			fprintf(stream, "%s", conf->value.s);
			break;
		case DICT:
			l = &conf->value.l;
			for (i = 0 ; i < l->length ; i++) {
				rv = yconfig_write_rec(l->elts[i], stream, n + INDENT);
				if (rv != 0)
					return rv;
			}
			break;
		case LIST:
			l = &conf->value.l;
			for (i = 0 ; i < l->length ; i++) {
				indent(stream, n + INDENT);
				fprintf(stream, "- ");
				rv = yconfig_write_rec(l->elts[i], stream, INDENT);
				if (rv != 0)
					return rv;
			}
			break;
		default:
			break;
	}

	fprintf(stream, "\n");

	return 0;
}

int yconfig_write(struct yconfig const * conf, FILE * stream)
{
	int rv;

	rv = yconfig_write_rec(conf, stream, 0);
	fflush(stream);

	return rv;
}

int yconfig_write_file(struct yconfig const * conf, char const * filename)
{
	int rv;
	FILE * stream;

	stream = fopen(filename, "w");
	if (stream == NULL)
		return -1;

	rv = yconfig_write(conf, stream);

	fclose(stream);
	return rv;
}

void yconfig_destroy(struct yconfig * conf)
{
	if (conf != NULL) {
		value_destroy(conf->type, &conf->value);
		free(conf->key);
		free(conf);
	}
}

int yconfig_get_list_length(struct yconfig * conf)
{
	assert(conf && conf->type == LIST);

	return conf->value.l.length;
}

struct yconfig * yconfig_get_list_elt(struct yconfig * conf, int i)
{
	assert(conf && conf->type == LIST);

	if (i < 0 || i >= conf->value.l.length)
		return NULL;

	return conf->value.l.elts[i];
}

static
struct yconfig * _yconfig_get_elt(struct yconfig * conf, char const * key, size_t keylen)
{
	int i;
	struct yconfig_list * l;

	assert(conf && (conf->type == LIST || conf->type == DICT));

	l = &conf->value.l;
	for (i = 0 ; i < l->length ; i++) {
		struct yconfig * elt = l->elts[i];
		if (strncmp(elt->key, key, keylen) == 0)
			return elt;
	}

	return NULL;
}

struct yconfig * yconfig_get_elt(struct yconfig * conf, char const * key)
{
	if (key == NULL)
		return NULL;

	return _yconfig_get_elt(conf, key, strlen(key));
}

int yconfig_get_int(struct yconfig * conf, int * value)
{
	char * ptr;
	long int tmp;

	if (conf->type != DATA)
		return -1;

	errno = 0;
	tmp = strtol(conf->value.s, &ptr, 10);

	if (ptr == conf->value.s
			|| (errno == ERANGE
				&& (tmp == LONG_MIN || tmp == LONG_MAX || tmp == 0))) {
		return -1;
	}

	*value = tmp;
	return 0;
}

int yconfig_get_float(struct yconfig * conf, float * value)
{
	char * ptr;
	float tmp;

	if (conf->type != DATA)
		return -1;

	errno = 0;
	tmp = strtof(conf->value.s, &ptr);

	if (ptr == conf->value.s
			|| (errno == ERANGE
				&& (tmp == HUGE_VALF || tmp == HUGE_VALL || tmp == 0))) {
		return -1;
	}

	*value = tmp;
	return 0;
}

int yconfig_get_bool(struct yconfig * conf, int * value)
{
	if (conf->type != DATA || conf->value.s == NULL)
		return -1;

	if (strlen(conf->value.s) > 7)
		return -1;

	if (strcasecmp(conf->value.s, "true") == 0
			|| strcasecmp(conf->value.s, "on") == 0
			|| strcasecmp(conf->value.s, "y") == 0
			|| strcasecmp(conf->value.s, "1") == 0) {
		*value = 1;
		return 0;
	}

	if (strcasecmp(conf->value.s, "false") == 0
			|| strcasecmp(conf->value.s, "off") == 0
			|| strcasecmp(conf->value.s, "n") == 0
			|| strcasecmp(conf->value.s, "0") == 0) {
		*value = 0;
		return 0;
	}

	return -1;
}

int yconfig_get_string(struct yconfig * conf, char ** value)
{
	if (conf->type != DATA)
		return -1;

	*value = conf->value.s;
	return 0;
}

__attribute__((nonnull(1)))
static inline char const * strchr_end(char const * s, int c)
{
	while (*s != c && *s != '\0')
		s++;

	return s;
}

static
struct yconfig * _yconfig_lookup_rec(struct yconfig * root, char const * key)
{
	char const * childkey, * end;
	size_t keylen, childkeylen;

	while (root != NULL) {
		end = strchr_end(key, ':');
		keylen = end - key;

		if (strncmp(root->key, key, keylen) != 0)
			return NULL;

		childkey = end + 1;
		end = strchr_end(childkey, ':');
		childkeylen = end - childkey;

		if (childkeylen == 0)
			return root;
		else
			return _yconfig_get_elt(root, childkey, childkeylen);
	}

	return NULL;
}

struct yconfig * yconfig_lookup(struct yconfig * root, char const * key)
{
	int i;
	struct yconfig_list * l;
	struct yconfig * rv;

	l = &root->value.l;
	for (i = 0 ; i < l->length ; i++) {
		rv = _yconfig_lookup_rec(l->elts[i], key);
		if (rv != NULL)
			return rv;
	}

	return NULL;
}

int yconfig_lookup_list_length(struct yconfig * root, char const * key)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_list_length(c);

	return -1;
}

struct yconfig * yconfig_lookup_list_elt(struct yconfig * root, char const * key, int i)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_list_elt(c, i);

	return NULL;
}

int yconfig_lookup_int(struct yconfig * root, char const * key, int * value)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_int(c, value);

	return -1;
}

int yconfig_lookup_float(struct yconfig * root, char const * key, float * value)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_float(c, value);

	return -1;
}

int yconfig_lookup_bool(struct yconfig * root, char const * key, int * value)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_bool(c, value);

	return -1;
}

int yconfig_lookup_string(struct yconfig * root, char const * key, char ** value)
{
	struct yconfig * c = yconfig_lookup(root, key);
	if (c != NULL)
		return yconfig_get_string(c, value);

	return -1;
}
