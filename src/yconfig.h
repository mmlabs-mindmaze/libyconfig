#ifndef YAML_CONFIG_HEADER
#define YAML_CONFIG_HEADER

#include <stdint.h>
#include <stdio.h>

struct yconfig;

struct yconfig * yconfig_create(void);
void yconfig_destroy(struct yconfig *conf);
struct yconfig * yconfig_read(FILE * stream);
struct yconfig * yconfig_read_file(char const * filename);
int yconfig_write(struct yconfig const * conf, FILE * stream);
int yconfig_write_file(struct yconfig const * conf, char const * filename);

/* "get" API: get from the current yconfig structure */
int yconfig_get_list_length(struct yconfig * conf);
struct yconfig * yconfig_get_list_elt(struct yconfig * conf, int i);
struct yconfig * yconfig_get_elt(struct yconfig * conf, char const * key);
int yconfig_get_int(struct yconfig * conf, int * value);
int yconfig_get_float(struct yconfig * conf, float * value);
int yconfig_get_bool(struct yconfig * conf, int * value);
int yconfig_get_string(struct yconfig * conf, char ** value);

/* "lookup" API: use a column concatenation of the keys as path to the key.
 * (column is an invalid char in yaml keys as it's part of the yaml language)
 * Eg:
 * key-1:
 *   key-2: value-2
 * lookup("key-1:key-2") -> value-2 */
struct yconfig * yconfig_lookup(struct yconfig * root, char const * key);
int yconfig_lookup_list_length(struct yconfig * root, char const * key);
struct yconfig * yconfig_lookup_list_elt(struct yconfig * root, char const * key, int i);
int yconfig_lookup_int(struct yconfig * root, char const * key, int * value);
int yconfig_lookup_float(struct yconfig * root, char const * key, float * value);
int yconfig_lookup_bool(struct yconfig * root, char const * key, int * value);
int yconfig_lookup_string(struct yconfig * root, char const * key, char ** value);

#endif /* YAML_CONFIG_HEADER */
