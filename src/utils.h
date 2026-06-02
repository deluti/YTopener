#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int directory_exists(const char *path);
int create_directory(const char *path);
int file_exists(const char *path);
int config_create();
int config_get_browser(char *browser, size_t size);
int config_set_browser(const char *browser);
void select_browser_menu(char *browser, size_t size);
void print_title();
int add_to_startup();
int remove_from_startup();
void open_youtube_video(const char *query, const char *browser);
void open_youtube_channel(const char *channel, const char *browser);
void run_console_mode();
void run_tray_mode();
void show_console_window();
void hide_console_window();

#ifdef __cplusplus
}
#endif
#endif