#include "client/appstate.hpp"
#include <curses.h>

// push an event onto the event queue
void push_event(Event event, std::queue<Event> &queue, std::mutex &lock) {
	lock.lock();
	queue.push(event);
	lock.unlock();
}

// drain all events currently in the queue into a vector for processing
std::vector<Event> drain_queue(std::queue<Event> &queue, std::mutex &lock) {
	lock.lock();
	size_t num_events = queue.size();
	std::vector<Event> output;
	output.reserve(num_events);
	while (!queue.empty()) {
		Event e = queue.front();
		queue.pop();
		output.push_back(e);
	}
	lock.unlock();
	return output;
}

// basic initialization for the terminal user interface
void init_tui(void) {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	nonl();
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
void cycle_focus(AppState &app, bool forward) {
	uint8_t nfocus = static_cast<uint8_t>(app.focus);

	if (forward) {
		nfocus = (nfocus % 3) + 1;
	} else {
		nfocus = (nfocus == 1)? 3 : nfocus - 1;
	}

	app.focus = static_cast<WindowFocus>(nfocus);

}

// returns a tuple of 3 Windows (in the order of the WindowFocus enum) of appropriate sizes given the overall screen height and width
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

// initialize an empty appstate
AppState init_appstate() {
    
    // initialize the tui
    init_tui();

	// get the height and width of the screen
	int height, width;
	getmaxyx(stdscr, height, width);

	// initialize the windows
	auto [contacts, chat_history, chat_in] = init_parent_windows(height, width);

	int h_contacts, w_contacts;
	getmaxyx(contacts, h_contacts, w_contacts);
	int h_history, w_history;
	getmaxyx(chat_history, h_history, w_history);
	int h_input, w_input;
	getmaxyx(chat_in, h_input, w_input);

	return {
		.contacts = contacts,
		.chat_history = chat_history,
		.chat_in = chat_in,
		.h_contacts = h_contacts,
		.w_contacts = w_contacts,
		.h_history = h_history,
		.w_history = w_history,
		.h_input = h_input,
		.w_input = w_input
	};
}

void put_temp_data(AppState &app, unsigned int num_users) {

	for (unsigned int i = 0; i < num_users; i++) {
		std::string username = "user_" + std::to_string(i);
		app.uids.push_back(i);
		app.uid_to_username[i] = username;
		app.username_to_uid[username] = i;
		app.chat_histories[i];
	}
}

// render the contacts sidebar
void render_contacts(AppState &app) {

	WINDOW *win = app.contacts;
	std::list<uid_t> contacts = app.uids;
	std::unordered_map<uid_t, std::string> username_map = app.uid_to_username;
	int selected_user = app.selected_user;
	int highlighted_user = app.highlighted_user;
	uint32_t sidebar_scroll = app.contacts_scroll;
	WindowFocus focus = app.focus;

	curs_set(0);
	
	// get the dimensions of the given window
	int height = app.h_contacts;
	int width = app.w_contacts;

	// erase the current window
	werase(win);

	// redraw the box based on whether or not it's in focus
	if (focus == WindowFocus::CONTACTS) {
		box(win, ACS_BLOCK, ACS_BLOCK);
	} else {
		box(win, 0, 0);
	}

	// how many rows do we need to render a single line?
	// for now let's say 2 with one extra at the top
	int v_offset = 1;
	int h_offset = 2;
	int rows_per_line = 2;
	int num_lines = (height - v_offset) / rows_per_line;

	// get an iterator for the list of contacts 
	std::list<uid_t>::iterator iter = contacts.begin();
	// account for scroll
	for (unsigned int i = 0; i < sidebar_scroll; i++) {
		if (iter != contacts.end()) {
			iter++;
		}
	}

	// render the lines
	for (int i = 0; i < num_lines; i++) {
		// line content to render;
		std::string line;
		// if it's the first line, just render the title
		if (i == 0) {
			line = "Contacts";
		
		// if not, then render the name
		} else {
			// check if the iterator is at end again, ending rendering if it is
			if (iter == contacts.end()) {
				break;
			}
			uid_t uid = *iter;
			iter++;
			line = username_map[uid];
		}
		// if this is the selected user, highlight it
		if ((i - 1) == selected_user) {
			wattron(win, A_STANDOUT);
			mvwprintw(win, (v_offset + rows_per_line * i), h_offset, "%s", line.c_str());
			wattroff(win, A_STANDOUT);
		
		// if it's the highlighted user, underline it
		} else if ((i - 1) == highlighted_user) {
			wattron(win, A_UNDERLINE);
			mvwprintw(win, (v_offset + rows_per_line * i), h_offset, "%s", line.c_str());
			wattroff(win, A_UNDERLINE);
				
		// otherwise, render it normally
		} else {
			mvwprintw(win, (v_offset + rows_per_line * i), h_offset, "%s", line.c_str());
		}
		
	}
	
	wrefresh(win);
	if (app.focus == WindowFocus::CHAT_IN) {
		curs_set(1);
	}
}

// render the chat input
void render_input(AppState &app) {

	// pointer to window for chat input
	WINDOW *win = app.chat_in;

	// height and width of the current window
	int height = app.h_input;
	int width = app.w_input;

	// erase the current window
	werase(win);

	// redraw the box based on whether or not it's in focus
	if (app.focus == WindowFocus::CHAT_IN) {
		box(win, ACS_BLOCK, ACS_BLOCK);
	} else {
		box(win, 0, 0);
	}

	// get the y pos of the text input, centered in window
	int y_pos = (height / 2) + (height % 2) - 1;

	// the idea is we decide where in the chat window we want the cursor
	// then we draw everything before the cursor, cutting off the prefix that wouldn't fit 
	// we draw everything after, cutting off the suffix that wouldn't fit 
	
	// it's honestly easier to do this using c strings, 
	// and we need to convert to that at the end anyway
	// so that's what we'll do
	const char *pre_c_str = app.pre_input_buffer.c_str();
	const char *post_c_str = app.post_input_buffer.c_str();

	// we'll also assume the cursor_pos is 0'd out AT THE TEXT OFFSET
	// if cursor pos is 0, then it is app.text_offset cells away from the border 

	// we increment the string view of the pre input buffer so that it will 
	// line up with what would be at the beginning of the window 
	pre_c_str += app.pre_input_buffer.size() - app.cursor_pos;

	// print the pre input buffer
	mvwprintw(win, y_pos, app.text_offset, "%s", pre_c_str);
	// print the post input buffer
	mvwprintw(win, y_pos, app.cursor_pos + app.text_offset, "%s", post_c_str);
	// place and draw the cursor if necessary
	
	wmove(win, y_pos, app.cursor_pos + app.text_offset);


	wrefresh(win);
	if (app.focus == WindowFocus::CHAT_IN) {
		curs_set(1);
	}
}

// render the chat history
void render_history(AppState &app) {

	curs_set(0);

	// pointer to window for chat input
	WINDOW *win = app.chat_history;

	// height and width of the current window
	int height = app.h_history;
	int width = app.w_history;

	// erase the current window
	werase(win);

	// redraw the box based on whether or not it's in focus
	if (app.focus == WindowFocus::CHAT_HIST) {
		box(win, ACS_BLOCK, ACS_BLOCK);
	} else {
		box(win, 0, 0);
	}

	wrefresh(win);
	if (app.focus == WindowFocus::CHAT_IN) {
		curs_set(1);
	}
}

