#include <string>
#include <vector>
#include <cstdint>
#include "client/client.hpp"
#include <curses.h>

// defines which window is currently in focus
enum class Window_Focus : uint8_t {
	USER_WINDOW = 1,
	CHAT_HISTORY = 2,
	CHAT_INPUT = 3
};

typedef struct _app_state {
    // users sidebar
    WINDOW* users_sidebar;
    std::vector<uid_t> users;
    // map of users 
    std::unordered_map<uid_t, std::string> user_map;

    // initialized to -1 to represent that there is no selected user
    int selected_user = -1;
    int highlight_user = -1;

    // how far down the sidebar is scrolled down
    uint32_t sidebar_scroll = 0;


    // chat history
    WINDOW* chat_history;
    std::unordered_map<uid_t, std::vector<std::string>> chats;

    // how far down the chat history is scrolled UP
    uint32_t chat_scroll = 0;

    // input
    WINDOW* chat_input;
    std::string input;

    // determines the focused window
    Window_Focus focus;
} AppState;


