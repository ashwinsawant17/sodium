#include "net/socket.hpp"
#include "protocol/message.hpp"
#include "net/client.hpp"

using namespace protocol;
using namespace net;

#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include <curses.h>

#define BUFF_SIZE 1024


// basic initialization for the terminal user interface
void init_tui(void) {
	setlocale(LC_ALL, "");
	initscr();
	cbreak();
	noecho();
	nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
}


// cleans up the terminal user interface
void cleanup_tui(void) {
	endwin();
}

// defines which window is currently in focus
enum class Window_Focus : uint8_t {
	USER_WINDOW = 1,
	CHAT_HISTORY = 2,
	CHAT_INPUT = 3
};

int main(void) {

	// initialize the tui
	init_tui();

	// set which window should be in focus at the beginning
	// for now, set the chat_input as the active window
	Window_Focus focus = Window_Focus::CHAT_INPUT;
	
	// render loop
	while (true) {
		int ch = getch();


		// q will simply exit the program
		if (ch == 'q') {
			break;

		// cycle forward
		} else if (ch == KEY_TAB) {
			focus++;
			focus %= 3;

		// cycle backward
		} else if (ch == KEY_BTAB) {
			focus--;
			focus = (static_cast<uint8_t>(focus) < 1)? 3 : focus;
		}
	}
	
	// cleanup the tui
	cleanup_tui();

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
