#define _GNU_SOURCE
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "transmit.h"
#include "xalt_quotestring.h"
#include "xalt_config.h"
#include "build_uuid.h"

#define DATESZ    100
#define HERE fprintf(stderr,"%s:%d\n",__FILE__,__LINE__)
#define DEBUG0(fp,s)             if (xalt_tracing) fprintf((fp),s)
#define DEBUG1(fp,s,x1)          if (xalt_tracing) fprintf((fp),s,(x1))
#define DEBUG2(fp,s,x1,x2)       if (xalt_tracing) fprintf((fp),s,(x1),(x2))
#define DEBUG3(fp,s,x1,x2,x3)    if (xalt_tracing) fprintf((fp),s,(x1),(x2),(x3))
#define DEBUG4(fp,s,x1,x2,x3,x4) if (xalt_tracing) fprintf((fp),s,(x1),(x2),(x3),(x4))

const  char*           xalt_syshost();

char* build_xalt_files_dir(const char* user_name, const char* home_dir, const char * transmission);

int main(int argc, char* argv[])
{
  char*       run_uuid	   = NULL;
  const char* my_host      = xalt_syshost();
  int         xalt_tracing = 0;
  int         len;
  char*       p_dbg;
  char        c;

  p_dbg = getenv("XALT_TRACING");
  if (p_dbg)
    xalt_tracing = (strcmp(p_dbg,"yes") == 0) || (strcmp(p_dbg,"pkg") == 0);

  while ((c = getopt(argc,argv, "s:u:")) != -1)
    {
      switch (c)
	{
	case 'u':
	  len      = strlen(optarg);
	  run_uuid = (char *) malloc(len+1);
	  memcpy(run_uuid, optarg, len+1);
	  break;
	}
    }
  
  if (! my_host)
    {
      DEBUG0(stderr,"Syshost is not set => quitting!\n");
      exit(0);
    }

  if (! run_uuid)
    {
      DEBUG0(stderr,"The run_uuid value is not set => quitting!\n");
      exit(0);
    }

  const char * transmission = getenv("XALT_TRANSMISSION_STYLE");
  if (transmission == NULL)
    transmission = TRANSMISSION;

  // Count size of string. Adjust count for possible UTF8 chars and two quotes and either a colon or a comma:

  //{"a":"b","c":"d"}
  int i;
  int sz = 2;  // {}
  for (i = optind; i < argc; ++i)
    sz += 3*strlen(argv[i]) + 3;

  char* json_str = (char *)malloc(sz+1);

  if ((argc - optind) % 2 == 1)
    {
      DEBUG1(stderr,"Odd number of parameter passed to %s => quiting!\n",argv[0]);
      exit(0);
    }


  const char *qs;
  int idx = 0;
  json_str[idx++] = '{';
  for (i = optind; i < argc; i+=2)
    {
      qs	      = xalt_quotestring(argv[i]);
      len	      = strlen(qs);
      json_str[idx++] = '"';
      memcpy(&json_str[idx], qs, len);
      idx	     += len;
      json_str[idx++] = '"';
      json_str[idx++] = ':';
      qs	      = xalt_quotestring(argv[i+1]);
      len	      = strlen(qs);
      json_str[idx++] = '"';
      memcpy(&json_str[idx], qs, len);
      idx	     += len;
      json_str[idx++] = '"';
      json_str[idx++] = ',';
    }
  json_str[--idx] = '}';
  json_str[++idx] = '\0';

  char* resultFn = NULL;
  
  char  uuid_str[37];
  build_uuid(uuid_str);
  //          1         2   |      3
  //0123456789 123456789 123456789 123456789 
  //3de2c9d8-e857-4482-9aa4-4979620380fc	  
  if ( (strcasecmp(transmission, "file") == 0) || (strcasecmp(transmission, "file_separate_dirs") == 0))
    {
      char* date_str = getenv("XALT_DATE_TIME");
      char* c_home   = getenv("HOME");
      char* c_user   = getenv("USER");

      if (c_home != NULL && c_user != NULL)
	{
          char* xalt_files_dir = build_xalt_files_dir(c_user, c_home, transmission);
	  
	  asprintf(&resultFn,"%spkg.%s.%s.%s.%s.json", xalt_files_dir, my_host, date_str,
		                                       run_uuid, &uuid_str[24]);
	  DEBUG1(stderr,"resultFn: %s\n",resultFn);
	  free(xalt_files_dir);
	}
    }
  
  char* key = NULL;
  asprintf(&key,"pkg_%s_%s",run_uuid, &uuid_str[24]);

  transmit(transmission, json_str, "pkg", key, my_host, resultFn);

  if (resultFn)
    free(resultFn);
  free(key);
  free(run_uuid);
  free(json_str);
  xalt_quotestring_free();

  return 0;
}

char* build_xalt_files_dir(const char* user_name, const char* home_dir, const char* transmission)
{
  char* xalt_files_dir = NULL;
  const char* pkg = "";

  if (strcasecmp(transmission,"file_separate_dirs") == 0)
    pkg = "pkg/";

#ifdef HAVE_FILE_PREFIX
  asprintf(&xalt_files_dir,"%s/%s%s/",XALT_FILE_PREFIX, pkg, user_name);
#else
  asprintf(&xalt_files_dir,"%s/.xalt.d/%s",home_dir, pkg);
#endif
  return xalt_files_dir;
}

