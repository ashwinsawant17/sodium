#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "client/client.hpp"

using namespace protocol;
using namespace net;

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <tuple>
#include <queue>
#include <mutex>
#include <curses.h>
#include <format>
#include <list>

#define BUFF_SIZE 1024

// defines which window is currently in focus
enum class Window_Focus : uint8_t {
	USER_WINDOW = 1,
	CHAT_HISTORY = 2,
	CHAT_INPUT = 3
};


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

enum class EventType {
	INC_MSG,
	OUT_MSG,
	NEW_CONTACT
};

struct Event {
	EventType type;
	uid_t user;
	std::string payload;
};

void push_event(Event event, std::queue<Event> &queue, std::mutex &lock) {
	lock.lock();
	queue.push(event);
	lock.unlock();
}

Event pop_event(std::queue<Event> &queue, std::mutex &lock) {
	lock.lock();
	Event e = queue.front();
	queue.pop();
	lock.unlock();
	return e;
}

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

void simulate_listener(int num_users, std::queue<Event> &queue, std::mutex &lock, bool *should_continue) {
	const unsigned int TIMEOUT_MS = 1000;
	unsigned int latest_msg = 0;
	uid_t last_user = 0;

	while (*should_continue) {
		// sleep for the designated timeout
		std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
		// choose a random uid_to_username
		std::string msg = std::format("message {}", latest_msg);
		latest_msg++;
		Event e = {EventType::INC_MSG, last_user, msg};
		last_user++;
		if (last_user >= num_users) {
			last_user = 0;
		}
	}
}

void render_contacts(
	WINDOW *win, 
	std::list<uid_t> contacts, 
	std::unordered_map<uid_t, std::string> username_map, 
	int selected_user, 
	int highlighted_user,
	uint32_t sidebar_scroll,
	Window_Focus focus
) {

	
	// get the dimensions of the given window
	int height, width;
	getmaxyx(win, height, width);

	// redraw the box based on whether or not it's in focus
	if (focus == Window_Focus::USER_WINDOW) {
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
	
}



int main(void) {

	
	// initialize the tui
	init_tui();

	// set which window should be in focus at the beginning
	// for now, set the chat_input as the active window
	Window_Focus focus = Window_Focus::USER_WINDOW;

	// set the input buffer
	std::string chat_input;

	// get the actual height and width of the main screen, will likely need to be updated later
	int height, width;
	getmaxyx(stdscr, height, width);

	// calls newwin with appropriate sizes
	auto [user_bar, chat_hist, chat_in] = init_parent_windows(height, width);

	if (!user_bar || !chat_hist || !chat_in) {
		std::cout << "window initialization is NULL?" << "\n";
		exit(1);
	}

	// before we start drawing, set up the data structures we'll be using
	// the idea:
	// an async thread will ONLY push to an event queue
	// the processing of events will happen within the same thread as rendering
	// not sure if this is the best idea, but it should hopefully avoid thread unsafety

	// a queue for events
	std::queue<Event> event_queue;
	// a lock on the event queue for thread safety
	std::mutex eq_mtx;

	// a mapping from uids to usernames and vice versa
	std::unordered_map<uid_t, std::string> uid_to_username;
	std::unordered_map<std::string, uid_t> username_to_uid;
	// fill the mapping with some filler data 
	int num_users = 32;
	for (int i = 0; i < num_users; i++) {
		std::string username = std::format("user{}", i);
		uid_to_username[i] = username;
		username_to_uid[username] = i;
	}

	// a mapping from uids to chat histories
	// a vector represents each chat history, where each element is a 
	// bool, string pair, where the string is the message, and 
	// the boolean is true if the sender is the client, and false otherwise, so we know which username to prepend to the message
	// TODO: find a way to change this up for things like group chats
	std::unordered_map<uid_t, std::vector<std::pair<bool, std::string>>> chat_histories;
	// fill the mapping with empty vectors to start
	for (int i = 0; i < num_users; i++) {
		// pretty sure this will assign the value for this key with a default constructor
		chat_histories[i];
	}

	// an ordered list of contacts
	std::list<uid_t> contacts;
	for (int i = 0; i < num_users; i++) {
		contacts.push_back(i);
	}



	// set borders for each window for now
	box(user_bar, 0, 0);
	box(chat_hist, 0, 0);
	box(chat_in, 0, 0);

	wrefresh(user_bar);
	wrefresh(chat_hist);
	wrefresh(chat_in);
	//refresh();
	int selected_user = 0;
	int highlighted_user = 1;
	// render loop
	while (true) {
		

		render_contacts(
			user_bar, 
			contacts, 
			uid_to_username, 
			selected_user, 
			highlighted_user,
			0, focus);
		
		
		box(chat_hist, 0, 0);
		box(chat_in, 0, 0);

		wrefresh(user_bar);
		wrefresh(chat_hist);
		wrefresh(chat_in);
		//refresh();

		int ch = getch();

		if (ch == 'q') break;
		
		if (ch == KEY_UP) {
			highlighted_user--;
			if (highlighted_user < 0) {
				highlighted_user = num_users - 1;
			}
		} else if (ch == KEY_DOWN) {
			highlighted_user++;
			if (highlighted_user >= num_users) {
				highlighted_user = 0;
			}
		} else if (ch == 13) {
			selected_user = highlighted_user;
		}
	}
	// cleanup the tui 
	cleanup_tui();
	

	//std::cout << height << ", " << width << "\n";
	
    // get the host:port from stdin
    std::string input;
    std::cout << "Enter the HOST:PORT\n";
    std::getline(std::cin, input);

    size_t pos = input.find(":");

    if (pos == std::string::npos) {
        throw std::runtime_error("Invalid format for host:port.\n");
    }

    // separate the host from the port
    std::string host = input.substr(0, pos);
    std::string port = input.substr(pos + 1);

    std::string username;
    std::cout << "Enter your username:\n";
    std::getline(std::cin, username);

    Client client = Client(host, port, username);

    // for now, begin a read loop, and just send all the bytes to the server
    bool should_continue = true;
    std::thread receiver_thread(&Client::listen, &client, &should_continue); 
    do {

        std::getline(std::cin, input);

        if (input != "/exit") {
            // TODO: add check for how many bytes are actually sent
            client.parse_user_input(input);
        }
    } while (input != "/exit");

    should_continue = false;
    receiver_thread.join();
    net::cleanup_sockets();

    return 0;
}
