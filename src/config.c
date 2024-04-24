#include <sys/stat.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "config.h"
#include "menu.h"
#include "util.h"
#include "ywm.h"

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

void read_config()
{	
  int ret = snprintf(ywm_config_path, sizeof(ywm_config_path), "%s/%s", ywm_path, CONFIG_FILE);
  if (ret < 0) {
    fprintf(stderr, "Error getting the YWM config directory path\n");
    exit(EXIT_FAILURE);
  }
  
  FILE *fp = open_file(ywm_config_path);
  if (fp == NULL) {
    fprintf(stderr, "Cannot open %s, creating default config\n", ywm_config_path);
	  write_default_config_file();
    fp = open_file(ywm_config_path);
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

void *poll_config_file()
{
  fprintf(stderr, "Starting config file polling thread\n");
  struct stat file_stat;
	time_t last_modified_time = 0;

	if (stat(ywm_config_path, &file_stat) == 0) {
		last_modified_time = file_stat.st_mtime;
	}
	
	while (1) {
	    if (stat(ywm_config_path, &file_stat) == 0) {
	        if (file_stat.st_mtime != last_modified_time) {
				    last_modified_time = file_stat.st_mtime;
		        fprintf(stderr, "Config updated\n");
            free(config);
            read_config();
            create_menu_title_gc();
            set_background();
	        }
	    } else {
	        fprintf(stderr, "Error getting file information: %s\n", ywm_config_path);
	    }	
		sleep(1);
	}
}
