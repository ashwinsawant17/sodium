#pragma once
#include <string>
#include <vector>
#include <list>
#include <queue>
#include <mutex>
#include <unordered_map>

#include <cstdint>
#include "client/client.hpp"
#include <curses.h>

// type of event on the async event queue
enum class EventType {
	INC_MSG,
	OUT_MSG,
	NEW_CONTACT
};

// event to be put on the async event queue
struct Event {
	EventType type;
	uid_t user;
	std::string payload;
};

// defines which window is currently in focus
enum class WindowFocus : uint8_t {
	CONTACTS = 1,
	CHAT_HIST = 2,
	CHAT_IN = 3
};

typedef struct _app_state {
    
    // overall window management
    // window structures for curses
    WINDOW *contacts;
    WINDOW *chat_history;
    WINDOW *chat_in;

    // currenty focused window
    WindowFocus focus = WindowFocus::CHAT_IN;

    // user management
    // known contacts 
    std::list<uid_t> uids;

    // mapping from uids to usernames
    std::unordered_map<uid_t, std::string> uid_to_username;
    
    // mapping from usernames to uids
    std::unordered_map<std::string, uid_t> username_to_uid;

    // a mapping from uids to chat histories
	// a vector represents each chat history, where each element is a 
	// bool, string pair, where the string is the message, and 
	// the boolean is true if the sender is the client, and false otherwise, so we know which username to prepend to the message
	// TODO: find a way to change this up for things like group chats
	std::unordered_map<uid_t, std::vector<std::pair<bool, std::string>>> chat_histories;


    // individual window level management: 
    // contacts window: 
    // scroll amount
    uint32_t contacts_scroll;
    // selected and highlighted users
    int selected_user = 0;
    int highlighted_user = 0;
    // dimensions of contacts window
    int h_contacts, w_contacts;
    // determine necessity to update contacts
    bool update_contacts = false;

    // chat history window: 
    // how far up we've scrolled in the selected chat history
    uint32_t history_scroll = 0;
    // dimensions of history window
    int h_history, w_history;
    // determine necessity to update chat history
    bool update_history = false;
    
    // chat input window: 
    // ofset to start drawing text
    int text_offset = 2;
    // local cursor pos in the window
    int cursor_pos = 0;
    // the string content before the cursor
    std::string pre_input_buffer;
    // the string content after the cursor
    std::string post_input_buffer;
    // dimensions of input window
    int h_input, w_input;
    // determine necessity to update chat input
    bool update_input = false;

    
    // asynchronous event management
    // queue of events
    std::queue<Event> e_queue;
    // mutex for thread safe access to event queue
    std::mutex lock;

    // determine necessity to update entire screen
    bool update_screen = false;

} AppState;


// push an event onto the event queue
void push_event(Event event, std::queue<Event> &queue, std::mutex &lock);

// drain all events currently in the queue into a vector for processing
std::vector<Event> drain_queue(std::queue<Event> &queue, std::mutex &lock);

// basic initialization for the terminal user interface
void init_tui(void);

// cleans up the terminal user interface
void cleanup_tui(void);

// cycles the focus between the windows
void cycle_focus(AppState &app, bool forward);

// returns a tuple of 3 Windows (in the order of the WindowFocus enum) of appropriate sizes given the overall screen height and width
std::tuple<WINDOW *, WINDOW *, WINDOW *> init_parent_windows(int height, int width);

// initialize an empty appstate
AppState init_appstate();

// generate some filler data 
void put_temp_data(AppState &app, unsigned int num_users);

// render the contacts sidebar
void render_contacts(AppState &app);

// render the chat input
void render_input(AppState &app);

// render the chat history
void render_history(AppState &app);