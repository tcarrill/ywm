#include <sys/stat.h>
#include <limits.h>
#include "config.h"

#define YWM_DIR ".ywm"
#define CONFIG_FILE "ywmrc"

char ywm_config_path[PATH_MAX];

void write_default_config_file() {
  const char* const default_config[] = {
	  "background_color=#666797",
	  "menu_title_color=#9b9bbb",
	  "window_resistance=30",
	  "window_snap_buffer=20",
	  "edge_resistance=50",
	  "edge_snap_buffer=20"
  };
  int status = mkdir_p(ywm_path);
  if (status == 0) {
    FILE *fp = fopen(ywm_config_path, "w");
    for (int i = 0; i < 6; i++) {
      fprintf(fp, "%s\n", default_config[i]);
    }
    fclose(fp);
  }
}

FILE* open_config_file(char *path) 
{
  FILE *fp = fopen(path, "r");
  return fp;
}

void read_config()
{	
  snprintf(ywm_path, sizeof(ywm_path), "%s/%s", getenv("HOME"), YWM_DIR);  
  snprintf(ywm_config_path, sizeof(ywm_config_path), "%s/%s", ywm_path, CONFIG_FILE);
  FILE *fp = open_config_file(ywm_config_path);
  if (fp == NULL) {
    fprintf(stderr, "Cannot open %s, creating default config\n", ywm_config_path);
	write_default_config_file();
    fp = open_config_file(ywm_config_path);
  }
 
  config = (YConfig *)malloc(sizeof(YConfig));
  
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
     
  int index = 0;
  while ((read = getline(&line, &len, fp)) != -1) {
    size_t ln = strlen(line) - 1;
    if (line[ln] == '\n') {
      line[ln] = '\0';
    }
	if (line[0] != '#') {
	    char *property = strtok(line, "=");
	    char *value = strtok(NULL, "=");
	
		if (strcmp(property, "window_resistance") == 0) {
			config->window_resistance = atoi(value);
		} else if (strcmp(property, "window_snap_buffer") == 0) {
			config->window_snap_buffer = atoi(value);	
		} else if (strcmp(property, "edge_resistance") == 0) {
			config->edge_resistance = atoi(value);
		} else if (strcmp(property, "edge_snap_buffer") == 0) {
			config->edge_snap_buffer = atoi(value);
		} else if (strcmp(property, "background_color") == 0) {
			strncpy(config->background_color, value, 8);
		} else if (strcmp(property, "menu_title_color") == 0) {
			strncpy(config->menu_title_color, value, 8);	
		}	
	}
	
    index++;
  }
	
  if (line) {
    free(line);
  }
  
  fclose(fp);	
}
