#include "jparse.h"
#define json_is_number(c)			\
  ((c) <= '9' && (c) >= '0')
#define json_is_alpha(c)			\
  ((c) <= 'z' && (c) >= 'a')
typedef struct parser parser_t;
struct parser
{
  char *code;
  char *cursor;
  char *start;
  
  json_t *json;
};

json_object_t *parse_elements (parser_t *ctx);
json_object_t *parse_element (parser_t *ctx);
json_array_t *parse_members (parser_t *ctx);
json_object_t *parse_object (parser_t *ctx);
json_array_t *parse_array (parser_t *ctx);
json_value_t *parse_value (parser_t *ctx);
json_t *parse_json (parser_t *ctx, char *data);
char *parse_string (parser_t *ctx);
double parse_number (parser_t *ctx);
json_type_t parse_logic (parser_t *ctx);
void skip (parser_t *ctx);
int advance (parser_t *ctx);
int peek (parser_t *ctx);
json_object_t *make_object (void);
json_value_t *make_value (void);
json_t *make_node (void);
json_t *json_decode (char *data);
json_array_t *make_array (void);

json_t *
json_decode (char *data)
{
  parser_t *ctx = malloc (sizeof (parser_t));
  json_t *json = parse_json (ctx, data);
  return json;
p}

json_t *
make_node (void)
{
  json_t *node = malloc (sizeof (json_t));
  return node;
}

json_value_t *
make_value (void)
{
  json_value_t *value = malloc (sizeof (json_value_t));
  return value;
}

json_object_t *
make_object (void)
{
  json_object_t *object = malloc (sizeof (json_object_t));
  return object;
}

json_array_t *
make_array (void)
{
  json_array_t *array = malloc (sizeof (json_array_t));
  return array;
}

int
peek (parser_t *ctx)
{
  return *ctx->cursor;
}

int
advance (parser_t *ctx)
{
  return *ctx->cursor++;
}

void
skip (parser_t *ctx)
{
  char c = peek (ctx);
  while (c == '\t' || c == '\r' ||
	 c == ' ' || c == '\n')
    {
      advance (ctx);
      c = peek (ctx);
    }
}

json_t *
parse_json (parser_t *ctx, char *data)
{
  ctx->code = data;
  ctx->cursor = ctx->code;
  ctx->start = ctx->code;

  ctx->json = make_node ();
  skip (ctx);
  
  if ((ctx->json->value = parse_value (ctx)) == NULL)
    return NULL;

  skip (ctx);
  return ctx->json;
}

json_value_t *
parse_value (parser_t *ctx)
{
  ctx->start = ctx->cursor;
  char c = advance (ctx);
  json_value_t *value = make_value ();

  switch (c)
    {
    case '{':
      value->type = JSON_OBJECT;
      value->as .object = parse_object (ctx);
      break;
    case '[':
      value->type = JSON_ARRAY;
      value->as.array = parse_array (ctx);
      break;
    case '"':
      value->type = JSON_STRING;
      value->as.string = parse_string (ctx);
      break;
    default:
      if (json_is_alpha (c))
	value->type = parse_logic (ctx);
      else if (json_is_number (c))
      	{
	  value->type = JSON_NUMBER;
      	  value->as.number = parse_number (ctx);
	}
      else
	return NULL;
    }
  return value;
}

double
parse_number (parser_t *ctx)
{
  while (json_is_number (peek (ctx)))
    advance (ctx);
  if (peek (ctx) == '.')
    advance (ctx);
  while (json_is_number (peek (ctx)))
    advance (ctx);
  int len = ctx->cursor - ctx->start;
  char *lexeme = malloc (len + 1);
  strncpy (lexeme, ctx->start, len);
  lexeme[len] = '\0';
  double val = strtod (lexeme, NULL);
  free (lexeme);
  return val;
}

json_type_t
parse_logic (parser_t *ctx)
{
  skip (ctx);
  while (json_is_alpha (peek (ctx)))
    advance (ctx);
  skip (ctx);
  if (strncmp (ctx->start, "true", 4) == 0)
    return JSON_TRUE;
  else if (strncmp (ctx->start, "false", 5) == 0)
    return JSON_FALSE;
  else if (strncmp (ctx->start, "null", 4) == 0)
    return JSON_NULL;
  else
    abort ();
}

json_array_t *
parse_array (parser_t *ctx)
{
  skip (ctx);
  json_array_t *array = NULL;

  skip (ctx);
  array = parse_members (ctx);
  skip (ctx);
  
  advance (ctx); /* trailing ']' */
  return array;
}

json_array_t *
parse_members (parser_t *ctx)
{
  json_array_t *arr = make_array ();
  json_array_t *curr = NULL;
  json_value_t *memb = NULL;
  if ((memb = parse_value (ctx)) == NULL)
    return NULL;
  
  arr->value = memb;
  arr->next = NULL;
  curr = arr;
  
  skip (ctx);
  while (peek (ctx) == ',')
    {
      advance (ctx);
      skip (ctx);
      curr->next = make_array ();
      curr = curr->next;
      curr->value = parse_value (ctx);
      curr->next = NULL; /* terminate linked list */
      skip (ctx);
    }
  advance (ctx); /* trailing ']' */
  return arr;
}      

json_object_t *
parse_object (parser_t *ctx)
{
  json_object_t *object = NULL;
  skip (ctx);

  object = parse_elements (ctx);
  /* trailing '}' */
  skip (ctx);
  advance (ctx);
  return object;
}

char *
parse_string (parser_t *ctx)
{
  ctx->start = ctx->cursor;
  while (peek (ctx) != '"')
    advance (ctx);
  int len = ctx->cursor - ctx->start;
  char *lexeme = malloc (len + 1);
  strncpy (lexeme, ctx->start, len);
  lexeme[len] = '\0';

  advance (ctx); /* trailing quote */
  return lexeme;
}

json_object_t *
parse_element (parser_t *ctx)
{
  json_value_t *value = NULL;
  json_object_t *object = make_object ();
  /* key */
  skip (ctx);
  if (advance (ctx) != '"')
    return NULL;
  char *key = parse_string (ctx);

  /* colon */
  skip (ctx);
  if (advance (ctx) != ':')
    return NULL;

  /* value */
  skip (ctx);
  if ((value = parse_value (ctx)) == NULL)
    return NULL;

  object->key = key;
  object->value = value;

  return object;
}

json_object_t *
parse_elements (parser_t *ctx)
{
  json_object_t *object = NULL;
  json_object_t *current = NULL;
  json_object_t *prev = NULL;
  if ((object = parse_element (ctx)) == NULL)
    return NULL;
  
  skip (ctx);
  current = object;
  while (peek (ctx) == ',')
    {
      prev = current;
      advance (ctx); /* eat ',' */
      skip (ctx);
      current->next = parse_element (ctx);
      current = current->next;
      skip (ctx);
    }
  return object;
}      
