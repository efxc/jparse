#include "jparse.h"
#define json_is_number(c)			\
  ((c) <= '9' && (c) >= '0')
#define json_is_alpha(c)			\
  ((c) <= 'z' && (c) >= 'a')

char *TOKENS[] =
  {"null", "false", "true", "object", "array",
   "string", "number", "error", NULL};

typedef struct parser parser_t;
struct parser
{
  char *code;
  char *cursor;
  char *start;

  int error;
  char *errmsg;
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
int end (parser_t *ctx);
json_object_t *make_object (void);
json_value_t *make_value (void);
json_t *make_node (void);
void make_error (parser_t *ctx, char *msg);
json_t *json_decode (char *data);
json_array_t *make_array (void);
void free_object (json_object_t *object);
void free_array (json_array_t *array);
int json_is_error (json_t *node);

json_t *
json_decode (char *data)
{
  parser_t *ctx = malloc (sizeof (parser_t));
  ctx->error = JSON_FALSE;
  ctx->errmsg = NULL;
  json_t *json = parse_json (ctx, data);
  free (ctx);
  return json;
}

void
json_free (json_t *json)
{
  if (json == NULL)
    return;
  json_object_t *object = NULL;
  json_array_t *array = NULL;
#ifdef DEBUG
  printf ("debug: [%p] removing '%s'\n", json, TOKENS[json->type]);
#endif
  switch (json->type)
    {
    case JSON_NULL:
    case JSON_TRUE:
    case JSON_FALSE:
    case JSON_NUMBER:
      free (json);
      break;
    case JSON_STRING:
      free (json->as.string);
      free (json);
      break;
    case JSON_OBJECT:
      object = json->as.object;
      free_object (object);
      break;
    case JSON_ARRAY:
      array = json->as.array;
      free_array (array);
      break;
    }
}

void
free_array (json_array_t *array)
{
  if (array != NULL)
    {
      free_array (array->next);
      array->next = NULL;
      json_free (array->value);
      free (array);
    }
}

void
free_object (json_object_t *object)
{
  if (object != NULL)
    {
      free_object (object->next);
      object->next = NULL;
      json_free (object->value);
#ifdef DEBUG
      printf ("debug: [%p] removing key '%s'\n", object, object->key);
#endif
      free (object->key);
    }
}

int
json_is_error (json_t *node)
{
  return
    node == NULL ||
    node->error == JSON_TRUE;
}

json_t *
make_node (void)
{
  json_t *node = malloc (sizeof (json_t));
  return node;
}

void
make_error (parser_t *ctx, char *msg)
{
  ctx->error = JSON_TRUE;
  ctx->errmsg = msg;
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
  if (!end (ctx))
    return *ctx->cursor++;

  return '\0';
}

int
end (parser_t *ctx)
{
  return peek (ctx) == '\0';
}

void
skip (parser_t *ctx)
{
  char c = peek (ctx);
  while ((c == '\t' || c == '\r' ||
	  c == ' ' || c == '\n') &&
	 c != '\0')
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
  
  ctx->json = parse_value (ctx);

  if (ctx->json == NULL)
    ctx->json = make_node ();
  
  ctx->json->error = ctx->error;
  ctx->json->errmsg = ctx->errmsg;
  skip (ctx);
  return ctx->json;
}

json_t *
parse_value (parser_t *ctx)
{
  ctx->start = ctx->cursor;
  char c = advance (ctx);
  json_t *value = make_node ();

  switch (c)
    {
    case '{':
      value->type = JSON_OBJECT;
      value->as.object = parse_object (ctx);
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
	{
	  make_error (ctx, "expected '{', '[', '\"' or literal"
			      " but got none of them");
	  value = NULL;
	}
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
    return JSON_ERROR;
}

json_array_t *
parse_array (parser_t *ctx)
{
  json_array_t *array = NULL;

  skip (ctx);
  array = parse_members (ctx);
  skip (ctx);
  
  if (advance (ctx) != ']') /* trailing ']' */
    {
      make_error (ctx, "expected ']'");
      array = NULL;
    }
  return array;
}

json_array_t *
parse_members (parser_t *ctx)
{
  json_array_t *arr = make_array ();
  json_array_t *curr = NULL;
  json_t *memb = NULL;
  
  skip (ctx);
  if (peek (ctx) == ']') /* no elements */
    {
      arr->value = NULL;
      return arr;
    }
  if (json_is_error (memb = parse_value (ctx)))
    {
      make_error (ctx, "expected value");
      return NULL;
    }
  skip (ctx);
  
  arr->value = memb;
  arr->next = NULL;
  curr = arr;
  
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
  skip (ctx);
  return arr;
}      

json_object_t *
parse_object (parser_t *ctx)
{
  json_object_t *object = NULL;

  skip (ctx);
  object = parse_elements (ctx);
  skip (ctx);

  /* trailing '}' */
  if (advance (ctx) != '}')
    {
      make_error (ctx, "unterminated object,"
		  " expected '}'");
      return NULL;
    }
  return object;
}

char *
parse_string (parser_t *ctx)
{
  ctx->start = ctx->cursor;
  while (peek (ctx) != '"' && !end (ctx))
    advance (ctx);
  int len = ctx->cursor - ctx->start;
  char *lexeme = malloc (len + 1);
  strncpy (lexeme, ctx->start, len);
  lexeme[len] = '\0';

  if (advance (ctx) != '"') /* trailing quote */
    return NULL;
  
  return lexeme;
}

json_object_t *
parse_element (parser_t *ctx)
{
  json_t *value = NULL;
  json_object_t *object = make_object ();
  /* key */
  skip (ctx);
  if (peek (ctx) != '"')
    return NULL;

  advance (ctx);
  char *key = parse_string (ctx);

  /* colon */
  skip (ctx);
  if (advance (ctx) != ':')
    {
      make_error (ctx, "expected ':'");
      return NULL;
    }

  /* value */
  skip (ctx);
  if (json_is_error (value = parse_value (ctx)))
    {
      make_error (ctx, "expected value, got none");
      return NULL;
    }

  object->key = key;
#ifdef DEBUG
  printf ("debug: [%p] created object with key '%s'\n",
	  object, object->key);
#endif
  object->value = value;

  skip (ctx);
  return object;
}

json_object_t *
parse_elements (parser_t *ctx)
{
  json_object_t *object = NULL;
  json_object_t *current = NULL;
  
  if ((object = parse_element (ctx)) == NULL)
    return NULL;
  
  skip (ctx);
  current = object;
  while (peek (ctx) == ',')
    {
      advance (ctx); /* eat ',' */
      skip (ctx);
      current->next = parse_element (ctx);
      if (current->next == NULL)
	{
	  make_error (ctx, "expected element");
	  return NULL;
	}
#ifdef DEBUG
      printf ("debug: [%p] parse_element returned address %p\n",
	      current, current->next);
#endif
      current = current->next;
      current->next = NULL;
      skip (ctx);
    }
  return object;
}      
