#include "jparse.h"

int spaces = 0;
void
print_spaces (void)
{
  for (int i = 0; i < spaces; ++i)
    putchar (' ');
}

void print_json (json_t *json);

void
print_object (json_object_t *object)
{
  json_object_t *current = object;
  if (current == NULL)
    return;
  
  spaces += 2;
  while (current != NULL)
    {
      puts ("");
      print_spaces ();
      printf ("'%s': ", current->key);
      print_json (current->value);
      current = current->next;
    }
  spaces -= 2;
}

void
print_array (json_array_t *array)
{
  json_array_t *current = array;
  if (current == NULL)
    return;

  spaces += 2;
  while (current != NULL)
    {
      puts ("");
      print_spaces ();
      print_json (current->value);
      current = current->next;
    }
  spaces -= 2;
}

void
print_json (json_t *json)
{
  if (json != NULL)
    {
      switch (json->type)
	{
	case JSON_NULL:
	  printf ("null ");
	  break;
	case JSON_TRUE:
	  printf ("true ");
	  break;
	case JSON_FALSE:
	  printf ("false ");
	  break;
	case JSON_OBJECT:
	  print_object (json->as.object);
	  break;
	case JSON_ARRAY:
	  print_array (json->as.array);
	  break;
	case JSON_NUMBER:
	  printf ("%g ", json->as.number);
	  break;
	case JSON_STRING:
	  printf ("'%s' ", json->as.string);
	  break;
	default:
	  break;
	}
    }
}

int
main (void)
{
  char source[10000];
  int nread = 0;
  while (!feof (stdin))
    nread += fread (source, 1, 999, stdin);
  source[nread] = '\0';

  json_data_t *json = json_decode (source);
  if (json->error == JSON_TRUE)
    {
      puts (json->reason);
      json_free (json->root);
      exit (EXIT_FAILURE);
    }
  print_json (json->root);
  puts ("");
  json_free (json->root);
}
