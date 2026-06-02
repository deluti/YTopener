#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "src/utils.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    char browser[256];
    
    if (!directory_exists("./.data")) {
        create_directory(".data");
    }
    
    if (!file_exists("./.data/config.yml")) {
        config_create();
    }
    
    config_get_browser(browser, sizeof(browser));
    
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