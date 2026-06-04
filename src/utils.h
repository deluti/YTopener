#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* get_config_dir(void);
const char* get_config_file_path(void);

int directory_exists(const char *path);
int create_directory(const char *path);
int file_exists(const char *path);
int config_create(void);
int config_get_browser(char *browser, size_t size);
int config_set_browser(const char *browser);
void select_browser_menu(char *browser, size_t size);
void print_title(void);
int add_to_startup(void);
int remove_from_startup(void);
void open_youtube_video(const char *query, const char *browser);
void open_youtube_channel(const char *channel, const char *browser);
void run_console_mode(void);
void run_tray_mode(void);
void show_console_window(void);
void hide_console_window(void);

#ifdef __cplusplus
}
#endif

#endif