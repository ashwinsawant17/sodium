#include "client/appstate.hpp"
#include <unordered_map>


// basic initialization for the terminal user interface
void init_tui(void) {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	keypad(stdscr, TRUE);
	nodelay(stdscr, TRUE);
	curs_set(0);
}

// cleans up the terminal user interface
void cleanup_tui(void) {
	endwin();
}

// cycles the focus between the windows
void cycle_focus(Window_Focus *focus, bool forward) {
	uint8_t nfocus = static_cast<uint8_t>(*focus);

	if (forward) {
		nfocus = (nfocus % 3) + 1;
	} else {
		nfocus = (nfocus == 1)? 3 : nfocus - 1;
	}

	*focus = static_cast<Window_Focus>(nfocus);
}

// returns a tuple of 3 Windows (in the order of the Window_Focus enum) of appropriate sizes given the overall screen height and width
std::tuple<WINDOW *, WINDOW *, WINDOW *> init_parent_windows(int height, int width) {

	// set the anchoring values for the windows
	const float usr_w_prop = 0.25f;
	const float in_h_prop = 0.125f;

	int usr_w = usr_w_prop * width;
	int in_h = in_h_prop * height;

	
	WINDOW* users_sidebar = newwin(height, usr_w, 0, 0);
	WINDOW* chat_history = newwin(height - in_h, width - usr_w, 0, usr_w);
	WINDOW* chat_input = newwin(in_h, width - usr_w, height - in_h, usr_w);


	return std::tuple<WINDOW *, WINDOW *, WINDOW *>(users_sidebar, chat_history, chat_input);
}

AppState init_appstate(std::unordered_map<uid_t, std::string> mapping) {
    
    // initialize the tui
    init_tui();

	// get the height and width of the screen
	int height, width;
	getmaxyx(stdscr, height, width);

	// initialize the windows
	auto [users_sidebar, chat_history, chat_in] = init_parent_windows(height, width);


    // window focus
    Window_Focus focus = Window_Focus::CHAT_INPUT;

    // chat input, by default makes an empty string
    std::string input;

    // user selection/highlight defaults
    int selected_user = -1;
    int highlight_user = -1;

    // how far down the sidebar is scrolled down
    uint32_t sidebar_scroll = 0;

    // the chat history
    std::unordered_map<uid_t, std::vector<std::string>> chats;

	// user sidebar
	std::vector<uid_t> users;

	// map of user ids to usernames
	std::unordered_map<uid_t, std::string> user_map = mapping;

    // creates an empty vector for each username known
    for (const auto& [key, value] : mapping) {
        chats[key];
		users.push_back(key);
    }

	return {
		.users_sidebar = users_sidebar,
		.users = users,
		.user_map = user_map,
		.chat_history = chat_history,
		.chats = chats,
		.chat_input = chat_in,
		.input = input,
		.focus = focus
	};
}

void draw_chat_in()