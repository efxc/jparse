#ifndef HAVE_JPARSER_H
#define HAVE_JPARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum json_type json_type_t;
typedef struct value json_t;
typedef struct value json_value_t;
typedef struct object json_object_t;
typedef struct array json_array_t;

enum json_type
  {
   JSON_NULL = 0,
   JSON_FALSE,
   JSON_TRUE,
   JSON_OBJECT,
   JSON_ARRAY,
   JSON_STRING,
   JSON_NUMBER,

   JSON_ERROR
  };

struct value
{
  json_type_t type;
  int error;
  char *errmsg;
  union
  {
    json_object_t *object;
    json_array_t *array;
    char *string;
    double number;
  } as;
};

struct object
{
  char *key;
  json_t *value;
  struct object *next;
};

struct array
{
  json_t *value;
  struct array *next;
};

json_t *json_decode (char *data);
void json_free (json_t *json);

#endif /* HAVE_JPARSER_H */
