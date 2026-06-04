#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "src/utils.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char browser[256] = {0};
    
    // Создаём папку конфига если её нет
    if (!directory_exists(get_config_dir())) {
        create_directory(get_config_dir());
    }
    
    if (!file_exists(get_config_file_path())) {
        config_create();
    }
    
    if (!config_get_browser(browser, sizeof(browser))) {
        strncpy(browser, "null", sizeof(browser) - 1);
        browser[sizeof(browser) - 1] = '\0';
    }
    
    if (strcmp(browser, "null") == 0) {
        AllocConsole();
        freopen("CONOUT$", "w", stdout);
        freopen("CONIN$", "r", stdin);
        
        print_title();
        select_browser_menu(browser, sizeof(browser));
        config_set_browser(browser);
        
        printf("\n>> Setup complete! YTopen will run in tray.\n");
        printf(">> Press Win+Y to open console.\n");
        printf(">> Press any key to start...");
        getchar();
        
        FreeConsole();
    }
    
    run_tray_mode();
    
    return 0;
}