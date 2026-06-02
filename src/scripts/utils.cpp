#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <stdlib.h>
#include "../utils.h"

using namespace Gdiplus;

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #define MKDIR(path) mkdir(path, 0755)
#endif

#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001

NOTIFYICONDATA nid;
HWND hwnd;
char current_browser[256];
static WNDPROC originalConsoleProc = NULL;
static ULONG_PTR gdiToken = 0;

// Convert console OEM/ANSI input to UTF-8 for URL encoding (handles Russian)
static void to_utf8(const char *src, char *dest, size_t destSize) {
    if (!src || !dest) return;
    dest[0] = '\0';
    int wideLen = MultiByteToWideChar(CP_OEMCP, 0, src, -1, NULL, 0);
    if (wideLen <= 0) return;
    wchar_t *wbuf = (wchar_t*)malloc(wideLen * sizeof(wchar_t));
    if (!wbuf) return;
    MultiByteToWideChar(CP_OEMCP, 0, src, -1, wbuf, wideLen);
    int utf8Len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (utf8Len <= 0) { free(wbuf); return; }
    if ((size_t)utf8Len > destSize) utf8Len = (int)destSize;
    WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, dest, utf8Len, NULL, NULL);
    dest[destSize-1] = '\0';
    free(wbuf);
}

LRESULT CALLBACK ConsoleWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_CLOSE) {
        ShowWindow(hWnd, SW_HIDE);
        return 0;
    }
    return CallWindowProc(originalConsoleProc, hWnd, msg, wParam, lParam);
}

static void subclass_console_window() {
    HWND console = GetConsoleWindow();
    if (console && originalConsoleProc == NULL) {
        originalConsoleProc = (WNDPROC)SetWindowLongPtr(console, GWLP_WNDPROC, (LONG_PTR)ConsoleWndProc);
    }
}

// load icon from assets/icon.ico or assets/icon.jpg using GDI+
static HICON load_custom_icon() {
    // try .ico first
    HICON hIcon = (HICON)LoadImageA(NULL, "assets\\icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE | LR_DEFAULTSIZE);
    if (hIcon) return hIcon;
    // init GDI+
    GdiplusStartupInput gdiInput;
    if (gdiToken == 0) {
        if (GdiplusStartup(&gdiToken, &gdiInput, NULL) != Ok) gdiToken = 0;
    }
    if (gdiToken == 0) return NULL;
    // load jpg via GDI+
    WCHAR wpath[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, "assets\\icon.jpg", -1, wpath, MAX_PATH);
    Bitmap *bmp = Bitmap::FromFile(wpath);
    if (!bmp || bmp->GetLastStatus() != Ok) {
        delete bmp;
        return NULL;
    }
    HBITMAP hBmp = NULL;
    if (bmp->GetHBITMAP(Color(0,0,0), &hBmp) != Ok) {
        delete bmp;
        return NULL;
    }
    ICONINFO ii = {0};
    ii.fIcon = TRUE;
    ii.hbmColor = hBmp;
    ii.hbmMask = CreateBitmap(32, 32, 1, 1, NULL);
    HICON hFromBmp = CreateIconIndirect(&ii);
    DeleteObject(ii.hbmMask);
    DeleteObject(hBmp);
    delete bmp;
    return hFromBmp;
}

LRESULT CALLBACK WindowProc(HWND hwndLocal, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            memset(&nid, 0, sizeof(nid));
            nid.cbSize = sizeof(NOTIFYICONDATA);
            nid.hWnd = hwndLocal;
            nid.uID = 1;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            HICON hCustom = load_custom_icon();
            if (hCustom) nid.hIcon = hCustom;
            else nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            strncpy(nid.szTip, "YTopen - YouTube Opener", sizeof(nid.szTip)-1);
            nid.szTip[sizeof(nid.szTip)-1] = '\0';
            Shell_NotifyIcon(NIM_ADD, &nid);
        } break;
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                HMENU hMenu = CreatePopupMenu();
                AppendMenuA(hMenu, MF_STRING, ID_TRAY_EXIT, "Exit");
                SetForegroundWindow(hwndLocal);
                TrackPopupMenu(hMenu, TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwndLocal, NULL);
                DestroyMenu(hMenu);
            }
            break;
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_TRAY_EXIT) {
                Shell_NotifyIcon(NIM_DELETE, &nid);
                PostQuitMessage(0);
                return 0;
            }
            break;
        case WM_HOTKEY:
            if (wParam == 1) {
                HWND existing = GetConsoleWindow();
                if (!existing) {
                    AllocConsole();
                    freopen("CONOUT$", "w", stdout);
                    freopen("CONIN$", "r", stdin);
                }
                subclass_console_window();
                ShowWindow(GetConsoleWindow(), SW_SHOW);
                SetForegroundWindow(GetConsoleWindow());
                run_console_mode();
                ShowWindow(GetConsoleWindow(), SW_HIDE);
            }
            break;
        case WM_DESTROY:
            Shell_NotifyIcon(NIM_DELETE, &nid);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwndLocal, uMsg, wParam, lParam);
    }
    return 0;
}

void hide_console_window() {
    HWND console = GetConsoleWindow();
    if (console) {
        ShowWindow(console, SW_HIDE);
    }
}

void show_console_window() {
    HWND console = GetConsoleWindow();
    if (console) {
        ShowWindow(console, SW_SHOW);
        SetForegroundWindow(console);
    }
}

void run_tray_mode() {
    WNDCLASSEX wc = {0};
    MSG msg;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "YTopenTrayClass";
    RegisterClassEx(&wc);
    hwnd = CreateWindowEx(0, "YTopenTrayClass", "YTopen", 0, 0, 0, 0, 0, NULL, NULL, wc.hInstance, NULL);
    if (!RegisterHotKey(hwnd, 1, MOD_WIN, 'Y')) {
        MessageBox(NULL, "Failed to register Win+Y hotkey!", "Error", MB_OK);
    }
    config_get_browser(current_browser, sizeof(current_browser));
    add_to_startup();
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnregisterHotKey(hwnd, 1);
}

int directory_exists(const char *path) {
    DIR *dir = opendir(path);
    if (dir) {
        closedir(dir);
        return 1;
    }
    return 0;
}

int create_directory(const char *path) {
    if (directory_exists(path)) {
        return 1;
    }
    return MKDIR(path) == 0;
}

int file_exists(const char *path) {
    FILE *file = fopen(path, "r");
    if (file) {
        fclose(file);
        return 1;
    }
    return 0;
}

int config_create() {
    const char *config_path = ".data/config.yml";
    FILE *file = fopen(config_path, "w");
    if (!file) return 0;
    fprintf(file, "browser: \"null\"\n");
    fclose(file);
    return 1;
}

int config_get_browser(char *browser, size_t size) {
    FILE *file = fopen(".data/config.yml", "r");
    if (!file) return 0;
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "browser:", 8) == 0) {
            char *value = line + 8;
            while (*value == ' ' || *value == '\t') value++;
            if (*value == '"') value++;
            char *end = strchr(value, '"');
            if (end) *end = '\0';
            char *newline = strchr(value, '\n');
            if (newline) *newline = '\0';
            strncpy(browser, value, size - 1);
            browser[size - 1] = '\0';
            fclose(file);
            return 1;
        }
    }
    fclose(file);
    return 0;
}

int config_set_browser(const char *browser) {
    FILE *file = fopen(".data/config.yml", "w");
    if (!file) return 0;
    fprintf(file, "browser: \"%s\"\n", browser);
    fclose(file);
    return 1;
}

void select_browser_menu(char *browser, size_t size) {
    int choice;
    printf("\n");
    printf("==================================================\n");
    printf("             SELECT YOUR BROWSER\n");
    printf("==================================================\n");
    printf("1. Google Chrome\n");
    printf("2. Mozilla Firefox\n");
    printf("3. Yandex Browser\n");
    printf("4. Microsoft Edge\n");
    printf("5. Opera\n");
    printf("6. Enter custom path\n");
    printf("==================================================\n");
    printf("Enter choice (1-6): ");
    scanf("%d", &choice);
    getchar();
    switch (choice) {
        case 1:
            strncpy(browser, "chrome", size);
            printf("\n>> Selected: Google Chrome\n");
            break;
        case 2:
            strncpy(browser, "firefox", size);
            printf("\n>> Selected: Mozilla Firefox\n");
            break;
        case 3:
            strncpy(browser, "yandex", size);
            printf("\n>> Selected: Yandex Browser\n");
            break;
        case 4:
            strncpy(browser, "edge", size);
            printf("\n>> Selected: Microsoft Edge\n");
            break;
        case 5:
            strncpy(browser, "opera", size);
            printf("\n>> Selected: Opera\n");
            break;
        case 6:
            printf("Enter browser path: ");
            fgets(browser, size, stdin);
            browser[strcspn(browser, "\n")] = 0;
            printf("\n>> Selected custom: %s\n", browser);
            break;
        default:
            strncpy(browser, "null", size);
            break;
    }
}

void print_title(){
    printf("==================================================\n");
    printf("                  YOUTUBE OPENER\n");
    printf("==================================================\n\n");
}

int add_to_startup() {
    char exe_path[MAX_PATH];
    char startup_path[MAX_PATH];
    char command[MAX_PATH * 2];
    GetModuleFileName(NULL, exe_path, MAX_PATH);
    snprintf(startup_path, sizeof(startup_path), "%s\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup", getenv("USERPROFILE"));
    snprintf(command, sizeof(command), "copy /Y \"%s\" \"%s\\YTopen.exe\"", exe_path, startup_path);
    return system(command) == 0;
}

int remove_from_startup() {
    char startup_path[MAX_PATH];
    char file_path[MAX_PATH];
    snprintf(startup_path, sizeof(startup_path), "%s\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup", getenv("USERPROFILE"));
    snprintf(file_path, sizeof(file_path), "%s\\YTopen.exe", startup_path);
    if (file_exists(file_path)) {
        return DeleteFile(file_path);
    }
    return 1;
}

void open_youtube_video(const char *query, const char *browser) {
    char url[1024];
    char command[2048];
    char enc[1024];
    char utf8[1024];
    to_utf8(query, utf8, sizeof(utf8));
    url[0] = '\0';
    enc[0] = '\0';
    const char *p = utf8;
    char *q = enc;
    while (*p && (size_t)(q - enc) < sizeof(enc) - 4) {
        unsigned char c = (unsigned char)*p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='-' || c=='_' || c=='.' || c=='~') {
            *q++ = c;
        } else if (c == ' ') {
            *q++ = '+';
        } else {
            sprintf(q, "%%%02X", c);
            q += 3;
        }
        p++;
    }
    *q = '\0';
    snprintf(url, sizeof(url), "https://www.youtube.com/results?search_query=%s", enc);
    if (strcmp(browser, "chrome") == 0) {
        snprintf(command, sizeof(command), "start chrome \"%s\"", url);
    } else if (strcmp(browser, "firefox") == 0) {
        snprintf(command, sizeof(command), "start firefox \"%s\"", url);
    } else if (strcmp(browser, "yandex") == 0) {
        snprintf(command, sizeof(command), "start yandex \"%s\"", url);
    } else if (strcmp(browser, "edge") == 0) {
        snprintf(command, sizeof(command), "start msedge \"%s\"", url);
    } else if (strcmp(browser, "opera") == 0) {
        snprintf(command, sizeof(command), "start opera \"%s\"", url);
    } else {
        snprintf(command, sizeof(command), "start \"\" \"%s\" \"%s\"", browser, url);
    }
    system(command);
    printf(">> Opening video: %s\n", query);
    // hide console after redirecting to browser
    hide_console_window();
}

void open_youtube_channel(const char *channel, const char *browser) {
    char url[1024];
    char command[2048];
    char enc[1024];
    char utf8[1024];
    to_utf8(channel, utf8, sizeof(utf8));
    const char *p = utf8;
    char *q = enc;
    while (*p && (size_t)(q - enc) < sizeof(enc) - 4) {
        unsigned char c = (unsigned char)*p;
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c=='-' || c=='_' || c=='.' || c=='~') {
            *q++ = c;
        } else if (c == ' ') {
            *q++ = '+';
        } else {
            sprintf(q, "%%%02X", c);
            q += 3;
        }
        p++;
    }
    *q = '\0';
    snprintf(url, sizeof(url), "https://www.youtube.com/results?search_query=%s", enc);
    if (strcmp(browser, "chrome") == 0) {
        snprintf(command, sizeof(command), "start chrome \"%s\"", url);
    } else if (strcmp(browser, "firefox") == 0) {
        snprintf(command, sizeof(command), "start firefox \"%s\"", url);
    } else if (strcmp(browser, "yandex") == 0) {
        snprintf(command, sizeof(command), "start yandex \"%s\"", url);
    } else if (strcmp(browser, "edge") == 0) {
        snprintf(command, sizeof(command), "start msedge \"%s\"", url);
    } else if (strcmp(browser, "opera") == 0) {
        snprintf(command, sizeof(command), "start opera \"%s\"", url);
    } else {
        snprintf(command, sizeof(command), "start \"\" \"%s\" \"%s\"", browser, url);
    }
    system(command);
    printf(">> Opening channel: %s\n", channel);
}

void run_console_mode() {
    char input[512];
    char browser[256];
    char arg[256];
    static int first_run = 1;
    config_get_browser(browser, sizeof(browser));
    if (first_run) {
        printf("\n");
        printf("==================================================\n");
        printf("              YTopen Console Mode\n");
        printf("==================================================\n");
        printf("Commands:\n");
        printf("  y <query>      - Search and open video (default)\n");
        printf("  y c <channel>  - Open YouTube channel\n");
        printf("  edit           - Change browser\n");
        printf("  exit           - Close console\n");
        printf("==================================================\n\n");
        first_run = 0;
    }
    while (1) {
        printf("ytopen> ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = 0;
        if (strcmp(input, "exit") == 0) {
            printf(">> Goodbye!\n");
            break;
        }
        if (strcmp(input, "edit") == 0) {
            select_browser_menu(browser, sizeof(browser));
            config_set_browser(browser);
            printf(">> Browser updated!\n");
            continue;
        }
        if (strncmp(input, "y ", 2) == 0) {
            // take everything after 'y '
            strncpy(arg, input + 2, sizeof(arg) - 1);
            arg[sizeof(arg)-1] = '\0';
            // trim leading/trailing spaces
            char *start = arg;
            while (*start == ' ') start++;
            char *end = start + strlen(start) - 1;
            while (end > start && (*end == ' ')) { *end = '\0'; end--; }
            // detect channel prefix: 'c' or 'channel'
            if ((start[0] == 'c' && start[1] == ' ') || strncmp(start, "channel ", 8) == 0) {
                // skip prefix
                char *ch = start;
                if (start[0] == 'c' && start[1] == ' ') ch = start + 2;
                else if (strncmp(start, "channel ", 8) == 0) ch = start + 8;
                while (*ch == ' ') ch++;
                open_youtube_channel(ch, browser);
                break;
            } else if ((start[0] == 'v' && start[1] == ' ') || strncmp(start, "video ", 6) == 0) {
                char *v = start;
                if (start[0] == 'v' && start[1] == ' ') v = start + 2;
                else if (strncmp(start, "video ", 6) == 0) v = start + 6;
                while (*v == ' ') v++;
                open_youtube_video(v, browser);
                break;
            } else {
                // default to video search
                open_youtube_video(start, browser);
                break;
            }
        }
        else {
            printf(">> Unknown command. Available: y <query>, y c <channel>, edit, exit\n");
        }
    }
}
