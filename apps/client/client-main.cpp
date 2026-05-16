#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "client/client.hpp"
#include "client/appstate.hpp"

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
#include <list>


#define BUFF_SIZE 1024

void simulate_listener(int num_users, std::queue<Event> &queue, std::mutex &lock, bool *should_continue) {
	const unsigned int TIMEOUT_MS = 2000;
	unsigned int latest_msg = 0;
	uid_t last_user = 0;

	while (*should_continue) {
		// sleep for the designated timeout
		std::this_thread::sleep_for(std::chrono::milliseconds(TIMEOUT_MS));
		// choose a random uid_to_username
		std::string msg = "message " + std::to_string(latest_msg);
		latest_msg++;
		Event e = {EventType::INC_MSG, last_user, msg};
		lock.lock();
		queue.push(e);
		lock.unlock();
		last_user++;
		if (last_user >= num_users) {
			last_user = 0;
		}
	}
}



int main(int argc, const char* argv[]) {

	std::string host, port, username;

	// validate that we have the right number of arguments
	if (argc < 3) {
		std::cerr << "Not enough arguments.\n";
	} else {
		// get the host:port
		std::string input(argv[1]); 
		size_t pos = input.find(":");

		if (pos == std::string::npos) {
			throw std::runtime_error("Invalid format for host:port.\n");
		}

		// separate the host from the port
		host = input.substr(0, pos);
		port = input.substr(pos + 1);

		username = std::string(argv[2]);
	}

	// initialize the client 
	Client client = Client(host, port, username);
	std::cerr << "Initialized client\n";


	// initialize the appstate
	AppState app = init_appstate();
	std::cerr << "Initialized appstate\n";


	bool should_continue = true;

	// start listener thread 
	std::thread listener_thread(&Client::listen, &client, &should_continue, 
		std::ref(app.e_queue), std::ref(app.lock));

	// do an initial render
	app.update_screen = true;
	refresh();
	
	// render loop
	while (true) {

		// process all the events
		// TODO: maybe wrap this in a safer interface?
		std::vector<Event> curr_events = drain_queue(app.e_queue, app.lock);
		
		// parse the events 
		parse_events(app, curr_events);

		// if the events queue was non-empty, we may need to update the screen 
		if (!curr_events.empty()) {
			app.update_screen = true;
		}

		if (app.update_screen) {
			render_contacts(app);
			render_history(app);
			render_input(app);
			app.update_screen = false;
		}

		int ch = getch();

		// reserved characters for global appstate changes or chars that should not change anything
		if (ch == ERR) {
			// TODO: this will skip all the processing in the rest of the loop
			// however, i'm not sure if we should also just skip this loop instead of skipping this iteration
			// for now, solve it by forcing any other processing that should happen regardless of character input to happen before the check
			continue;	
		} else {
			app.update_screen = true;
		}
		
		if (ch == 27) {
			should_continue = false;
			break;
		} else if (ch == KEY_RESIZE) {
			int height, width;
			getmaxyx(stdscr, height, width);
			resize_screen(app, height, width);
		} else if (ch == '\t'){
			cycle_focus(app, true);
		} else if (ch == KEY_BTAB) {
			cycle_focus(app, false);
		}
		
		// character input for specific windows
		// if the focus is currently on CONTACTS
		else if (app.focus == WindowFocus::CONTACTS){
			// first determine if w

			if (ch == KEY_UP) {
				app.highlighted_user--;
				if (app.highlighted_user < 0) {
					app.highlighted_user = app.uids.size() - 1;
				}
			} else if (ch == KEY_DOWN) {
				app.highlighted_user++;
				if (app.highlighted_user >= app.uids.size()) {
					app.highlighted_user = 0;
				}
			} else if (ch == 13) {
				app.selected_user = app.highlighted_user;
				app.chats_read[app.uids[app.selected_user]] = true;
			}
		
		// if the focus is currently on chat input
		} else if (app.focus == WindowFocus::CHAT_IN) {
			// moving cursor right
			if (ch == KEY_RIGHT) {
				// first check to make sure there is actually text to the right
				if (app.post_input_buffer.size() > 0) {
					// if there is, move the character accordingly
					char c = app.post_input_buffer.front();
					app.post_input_buffer.erase(app.post_input_buffer.begin());
					app.pre_input_buffer.push_back(c);

					// check to see if we can actually move the cursor one over
					if (app.cursor_pos + app.text_offset < app.w_input) {
						app.cursor_pos += 1;
					}
				}
			// moving cursor left
			} else if (ch == KEY_LEFT) {
				// use a similar approach to KEY_RIGHT 
				if (app.pre_input_buffer.size() > 0) {
					char c = app.pre_input_buffer.back();
					app.pre_input_buffer.pop_back();
					app.post_input_buffer.insert(app.post_input_buffer.begin(), c);
				
					// check to see if we can move the cursor back 
					if (app.cursor_pos > 0) {
						app.cursor_pos -= 1;
					}
				}
			// sending the message
			} else if (ch == 13) {
				// construct the total string buffer
				std::string msg = app.pre_input_buffer + app.post_input_buffer;
				// check if the selected user is valid 
				if (app.selected_user < app.uids.size()) {
					// add the message to the vector of strings for the selected user 
					app.chat_histories[app.uids[app.selected_user]].push_back({true, msg});
				}

				// have the client send the message to the appropriate user 
				client.parse_user_input(msg, app);

				// clean the buffer and reset the cursor
				app.pre_input_buffer.clear();
				app.post_input_buffer.clear();
				app.cursor_pos = 0;	
			} else if (ch == KEY_BACKSPACE) {
				// similar to KEY_LEFT, check if there's space to the left of the cursor
				if (app.pre_input_buffer.size() > 0) {
					app.pre_input_buffer.pop_back();
					// if we can, move the cursor left 
					if (app.cursor_pos > 0) {
						app.cursor_pos -= 1;
					}
				}
			} else {
				// if it's a normal character, insert it into the string 
				// im hoping that this truncates the character correctly
				char c = ch;
				app.pre_input_buffer.push_back(c);
				// if we can move the cursor forward, do it
				if (app.cursor_pos + app.text_offset < app.w_input) {
					app.cursor_pos += 1;
				}
			}
		}

		
	}
	// cleanup the tui 
	cleanup_tui();

	// end the thread
	listener_thread.join();

    net::cleanup_sockets();

    return 0;
}
