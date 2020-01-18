#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include "jparse.h"

static size_t
get_data (void *dest, size_t size, size_t nmemb, void *userp)
{
  memcpy (userp, dest, size * nmemb);
  return size * nmemb;
}  

int
main (int argc,
      char **argv)
{
  if (argc != 3)
    {
      fprintf (stderr, "error, usage: %s <city> <api key>\n", argv[0]);
      exit (EXIT_FAILURE);
    }

  char *appid = argv[2];
  char query[256] = "https://api.openweathermap.org/data/2.5/weather?q=";
  char response[2048];

  /* build the query string */
  strncat (query, argv[1], strlen (argv[1]));
  strncat (query, "&appid=", 8);
  strncat (query, appid, strlen (appid));

  CURL *handle = curl_easy_init ();
  CURLcode result = CURLE_OK;

  curl_easy_setopt (handle, CURLOPT_URL, query);
  curl_easy_setopt (handle, CURLOPT_FOLLOWLOCATION, 1L);
  curl_easy_setopt (handle, CURLOPT_WRITEFUNCTION, get_data);
  curl_easy_setopt (handle, CURLOPT_WRITEDATA, response);

  result = curl_easy_perform (handle);
  if (result != CURLE_OK)
    return 1;

  json_t *data = json_decode (response);
  if (data == NULL)
    return 1;
  char *p;
  json_object_t *val = data->as.object;
  while (val != NULL)
    {
      if (strcmp ("cod", val->key) == 0)
	break;
      val = val->next;
    }

  if (val->value->as.number != 200)
    {
      puts ("error occured.");
      return 1;
    }
  val = data->as.object;
  while (val != NULL)
    {
      if (strcmp ("weather", val->key) == 0)
	break;
      val = val->next;
    }

  json_object_t *weather = val->value->as.array->value->as.object;

  val = weather;
  while (val != NULL)
    {
      if (strcmp ("main", val->key) == 0)
	break;
      val = val->next;
    }

  puts (val->value->as.string);
  curl_easy_cleanup (handle);
}
