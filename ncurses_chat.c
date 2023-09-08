#include <arpa/inet.h>
#include <ctype.h>
#include <ncurses.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// Function definitions
WINDOW *create_win(int height, int width, int starty, int startx);
void readline(WINDOW *win, char *buffer, int buflen);
void chat_message(WINDOW *win, char *sender, char *message);
_Noreturn void *receive(void *vargp);

// Constants
const uint16_t PORT        = 2000;
const char *DEST_IP        = "192.168.1.21";
const int MAX_MESSAGE_SIZE = 2000;

typedef struct {
  int socket_desc;
  WINDOW *top_win;
} thread_args;

int main(void) {
  WINDOW *top_win, *bottom_win;
  struct sockaddr_in host_addr, dest_addr;
  int socket_desc;
  pthread_t thread_id;

  /*
   * Setup network stuff
   */

  // Set host information
  host_addr.sin_family      = AF_INET;
  host_addr.sin_port        = htons(PORT);
  host_addr.sin_addr.s_addr = INADDR_ANY;

  // Set destination information
  dest_addr.sin_family      = AF_INET;
  dest_addr.sin_port        = htons(PORT);
  dest_addr.sin_addr.s_addr = inet_addr(DEST_IP);

  // Create UDP socket
  socket_desc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (socket_desc < 0) {
    printf("Error while creating socket\n");
    return -1;
  }

  // Bind to the set port and HOST_IP
  if (bind(socket_desc, (struct sockaddr *)&host_addr, sizeof(host_addr)) < 0) {
    printf("Couldn't bind to the port\n");
    return -1;
  }

  /*
   * Setup ncurses stuff
   */
  initscr();// Start curses mode
  cbreak(); // Line buffering disabled
  noecho(); // Don't echo while taking input
  refresh();// Refresh the screen

  // Create windows
  top_win    = create_win(LINES * 0.8, COLS, 0, 0);
  bottom_win = create_win(LINES * 0.2, COLS, LINES * 0.8, 0);
  scrollok(top_win, TRUE);
  keypad(bottom_win, TRUE);

  // struct with socket_desc and top_win
  thread_args args = {socket_desc, top_win};

  // Receive client's message
  pthread_create(&thread_id, NULL, receive, &args);

  // Send message loop
  while (1) {
    // Create buffer
    char *message = (char *)calloc(MAX_MESSAGE_SIZE, sizeof(char));

    // Get input from the user
    wprintw(bottom_win, "[You] ");
    readline(bottom_win, message, MAX_MESSAGE_SIZE);

    // Send the message to server
    sendto(socket_desc, message, strlen(message), 0,
           (struct sockaddr *)&dest_addr, sizeof(dest_addr));

    // Add the message to the chat
    chat_message(top_win, "You", message);

    // Clear the input window
    wclear(bottom_win);
    wrefresh(bottom_win);

    // Free the buffer
    free(message);
  }

  endwin();// End curses mode
  // Close the socket
  close(socket_desc);

  return 0;
}

_Noreturn void *receive(void *vargp) {
  int socket_desc = ((thread_args *)vargp)->socket_desc;
  WINDOW *top_win = ((thread_args *)vargp)->top_win;
  struct sockaddr_in from_addr;
  socklen_t client_struct_length = sizeof(from_addr);
  char *received_message;

  // Receive loop
  while (1) {
    // Clean buffers
    received_message = (char *)calloc(MAX_MESSAGE_SIZE, sizeof(char));

    // Receive client's message
    recvfrom(socket_desc, received_message, MAX_MESSAGE_SIZE * sizeof(char), 0,
             (struct sockaddr *)&from_addr, &client_struct_length);

    // Print client's message
    char sender[50];
    snprintf(sender, 50, "%s:%i", inet_ntoa(from_addr.sin_addr),
             ntohs(from_addr.sin_port));
    chat_message(top_win, sender, received_message);

    // Free the buffer
    free(received_message);
  }
}

void chat_message(WINDOW *win, char *sender, char *message) {
  wprintw(win, "[%s] %s\n", sender, message);
  wrefresh(win);
}

/* adapted from https://stackoverflow.com/a/5232504/8161733 */
void readline(WINDOW *win, char *buffer, int buflen) {
  int old_curs = curs_set(1), pos = 0, len = 0, y, x;
  getyx(win, y, x);
  for (;;) {
    int c;

    buffer[len] = ' ';
    mvwaddnstr(win, y, x, buffer, len + 1);
    wmove(win, y, x + pos);
    c = wgetch(win);

    if (c == KEY_ENTER || c == '\n' || c == '\r') {
      break;
    } else if (isprint(c)) {
      if (pos < buflen - 1) {
        memmove(buffer + pos + 1, buffer + pos, len - pos);
        buffer[pos++] = c;
        len += 1;
      } else {
        beep();
      }
    } else if (c == KEY_LEFT) {
      if (pos > 0) pos -= 1;
      else
        beep();
    } else if (c == KEY_RIGHT) {
      if (pos < len) pos += 1;
      else
        beep();
    } else if (c == KEY_BACKSPACE) {
      if (pos > 0) {
        memmove(buffer + pos - 1, buffer + pos, len - pos);
        pos -= 1;
        len -= 1;
      } else {
        beep();
      }
    } else if (c == KEY_DC) {
      if (pos < len) {
        memmove(buffer + pos, buffer + pos + 1, len - pos - 1);
        len -= 1;
      } else {
        beep();
      }
    } else {
      beep();
    }
  }
  buffer[len] = '\0';
  if (old_curs != ERR) curs_set(old_curs);
}

WINDOW *create_win(int height, int width, int starty, int startx) {
  WINDOW *border_win, *actual_win;
  // Create a border window
  border_win = newwin(height, width, starty, startx);
  box(border_win, 0, 0);
  wrefresh(border_win);

  // Create an actual window (we don't want the border to be overwritten)
  actual_win = newwin(height - 2, width - 2, starty + 1, startx + 1);
  wrefresh(actual_win);

  return actual_win;
}