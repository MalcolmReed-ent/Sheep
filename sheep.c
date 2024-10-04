#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <mpd/client.h>
#include <time.h>
#include <ctype.h>
#include <sys/time.h>
#include <signal.h>
#include <locale.h>
#include <unistd.h>
#include "config.h"

typedef struct {
    char name[MAX_LENGTH];
    char uri[MAX_LENGTH];
} Item;

typedef struct {
    Item items[MAX_ITEMS];
    int count;
    int selected;
    int scroll_offset;
} Column;

Column artists, albums, songs;
int current_column = 0;
char search_term[MAX_LENGTH] = "";
int search_mode = 0;
char error_message[MAX_LENGTH] = "";
time_t error_display_time = 0;

WINDOW *main_win, *status_win;
struct mpd_connection *conn;

struct timeval last_update_time, last_search_time;

void init_mpd() {
    conn = mpd_connection_new(MPD_HOST, MPD_PORT, MPD_TIMEOUT);
    if (mpd_connection_get_error(conn) != MPD_ERROR_SUCCESS) {
        snprintf(error_message, MAX_LENGTH, "MPD connection error: %s", mpd_connection_get_error_message(conn));
        error_display_time = time(NULL);
    }
}

void fetch_artists() {
    artists.count = 0;
    struct mpd_pair *pair;
    mpd_search_db_tags(conn, MPD_TAG_ARTIST);
    mpd_search_commit(conn);
    while ((pair = mpd_recv_pair_tag(conn, MPD_TAG_ARTIST)) != NULL) {
        strncpy(artists.items[artists.count].name, pair->value, MAX_LENGTH - 1);
        artists.count++;
        mpd_return_pair(conn, pair);
        if (artists.count >= MAX_ITEMS) break;
    }
}

void fetch_albums(const char *artist) {
    albums.count = 0;
    struct mpd_pair *pair;
    mpd_search_db_tags(conn, MPD_TAG_ALBUM);
    mpd_search_add_tag_constraint(conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, artist);
    mpd_search_commit(conn);
    while ((pair = mpd_recv_pair_tag(conn, MPD_TAG_ALBUM)) != NULL) {
        strncpy(albums.items[albums.count].name, pair->value, MAX_LENGTH - 1);
        albums.count++;
        mpd_return_pair(conn, pair);
        if (albums.count >= MAX_ITEMS) break;
    }
}

void fetch_songs(const char *artist, const char *album) {
    songs.count = 0;
    struct mpd_song *song;
    mpd_search_db_songs(conn, true);
    mpd_search_add_tag_constraint(conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ARTIST, artist);
    mpd_search_add_tag_constraint(conn, MPD_OPERATOR_DEFAULT, MPD_TAG_ALBUM, album);
    mpd_search_commit(conn);
    while ((song = mpd_recv_song(conn)) != NULL) {
        const char *title = mpd_song_get_tag(song, MPD_TAG_TITLE, 0);
        const char *uri = mpd_song_get_uri(song);
        if (title && uri) {
            strncpy(songs.items[songs.count].name, title, MAX_LENGTH - 1);
            strncpy(songs.items[songs.count].uri, uri, MAX_LENGTH - 1);
            songs.count++;
        }
        mpd_song_free(song);
        if (songs.count >= MAX_ITEMS) break;
    }
}

void draw_progress_bar(int y, int x, int width, float progress) {
    int filled_width = (int)(progress * (width - 1));
    mvwhline(status_win, y, x, PROGRESS_BAR_BG, width);
    mvwhline(status_win, y, x, PROGRESS_BAR_CHAR, filled_width);
    mvwaddch(status_win, y, x + filled_width, PROGRESS_BAR_CURSOR);
}

void draw_status_bar() {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        wclear(status_win);
        int width;
        getmaxyx(status_win, (int){0}, width);
        
        if (SHOW_PLAYBACK_MODE) {
            char mode[20] = "";
            if (mpd_status_get_repeat(status)) strcat(mode, "repeat ");
            if (mpd_status_get_random(status)) strcat(mode, "random ");
            if (mpd_status_get_single(status)) strcat(mode, "single ");
            if (mpd_status_get_consume(status)) strcat(mode, "consume ");
            mvwprintw(status_win, 0, 0, "Mode: %s", mode);
        }
        
        if (SHOW_VOLUME) {
            mvwprintw(status_win, 0, width - 15, "Vol: %d%%", mpd_status_get_volume(status));
        }
        
        if (SHOW_PROGRESS_BAR) {
            int song_duration = mpd_status_get_total_time(status);
            int elapsed_time = mpd_status_get_elapsed_time(status);
            float progress = song_duration > 0 ? (float)elapsed_time / song_duration : 0;
            draw_progress_bar(1, 0, width, progress);
            
            if (SHOW_TIME_ELAPSED) {
                mvwprintw(status_win, 1, 0, "%d:%02d", elapsed_time / 60, elapsed_time % 60);
            }
            if (SHOW_TIME_TOTAL) {
                mvwprintw(status_win, 1, width - 7, "%d:%02d", song_duration / 60, song_duration % 60);
            }
        }
        
        struct mpd_song *current_song = mpd_run_current_song(conn);
        if (current_song) {
            const char *title = mpd_song_get_tag(current_song, MPD_TAG_TITLE, 0);
            const char *artist = mpd_song_get_tag(current_song, MPD_TAG_ARTIST, 0);
            mvwprintw(status_win, 2, 0, "Now Playing: %s - %s", artist ? artist : "Unknown", title ? title : "Unknown");
            
            if (SHOW_BITRATE) {
                mvwprintw(status_win, 2, width - 20, "Bitrate: %d kbps", mpd_status_get_kbit_rate(status));
            }
            
            if (SHOW_SAMPLE_RATE) {
                unsigned int sample_rate = mpd_song_get_duration(current_song);
                if (sample_rate > 0) {
                    mvwprintw(status_win, 2, width - 40, "Sample rate: %d Hz", sample_rate);
                }
            }
            
            mpd_song_free(current_song);
        }
        
        mpd_status_free(status);
    }
    wrefresh(status_win);
}

void draw_column(WINDOW *win, Column *col, int x, int width, const char *title, int is_active) {
    int height;
    getmaxyx(win, height, (int){0});
    
    wattron(win, COLOR_PAIR(is_active ? 3 : 4));
    mvwvline(win, 0, x + width - 1, ACS_VLINE, height);
    mvwhline(win, 1, x, ACS_HLINE, width - 1);
    mvwprintw(win, 0, x + (width - strlen(title)) / 2, "%s", title);
    wattroff(win, COLOR_PAIR(is_active ? 3 : 4));
    
    int display_count = height - 3;
    for (int i = 0; i < display_count && i + col->scroll_offset < col->count; i++) {
        int item_index = i + col->scroll_offset;
        int is_selected = (item_index == col->selected);
        
        if (is_selected && is_active) {
            wattron(win, COLOR_PAIR(2) | A_BOLD);
        } else if (is_selected) {
            wattron(win, COLOR_PAIR(5));
        } else if (is_active) {
            wattron(win, COLOR_PAIR(3));
        } else {
            wattron(win, COLOR_PAIR(1));
        }
        
        mvwprintw(win, i + 2, x, "%.*s", width - 2, col->items[item_index].name);
        
        if (is_selected && is_active) {
            wattroff(win, COLOR_PAIR(2) | A_BOLD);
        } else if (is_selected) {
            wattroff(win, COLOR_PAIR(5));
        } else if (is_active) {
            wattroff(win, COLOR_PAIR(3));
        } else {
            wattroff(win, COLOR_PAIR(1));
        }
    }
}

void draw_main_window() {
    wclear(main_win);
    int width, height;
    getmaxyx(main_win, height, width);
    
    int artist_width = width * ARTIST_COLUMN_WIDTH / 100;
    int album_width = width * ALBUM_COLUMN_WIDTH / 100;
    int song_width = width - artist_width - album_width;
    
    draw_column(main_win, &artists, 0, artist_width, "Artists", current_column == 0);
    draw_column(main_win, &albums, artist_width, album_width, "Albums", current_column == 1);
    draw_column(main_win, &songs, artist_width + album_width, song_width, "Songs", current_column == 2);
    
    if (search_mode) {
        wattron(main_win, COLOR_PAIR(7));
        mvwprintw(main_win, height - 1, 0, "Search: %s", search_term);
        wattroff(main_win, COLOR_PAIR(7));
    }
    
    if (error_message[0] != '\0' && time(NULL) - error_display_time < ERROR_DISPLAY_DURATION) {
        wattron(main_win, COLOR_PAIR(8));
        mvwprintw(main_win, height - 1, 0, "Error: %s", error_message);
        wattroff(main_win, COLOR_PAIR(8));
    }
    
    wrefresh(main_win);
}

void draw_screen() {
    draw_main_window();
    draw_status_bar();
}

void play_selected_song() {
    if (current_column == 2) {
        mpd_run_clear(conn);
        mpd_run_add(conn, songs.items[songs.selected].uri);
        mpd_run_play(conn);
    }
}

void toggle_playback() {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        if (mpd_status_get_state(status) == MPD_STATE_PLAY) {
            mpd_run_pause(conn, true);
        } else {
            mpd_run_play(conn);
        }
        mpd_status_free(status);
    }
}

void change_volume(int delta) {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        int current_volume = mpd_status_get_volume(status);
        int new_volume = current_volume + delta;
        if (new_volume < 0) new_volume = 0;
        if (new_volume > 100) new_volume = 100;
        mpd_run_set_volume(conn, new_volume);
        mpd_status_free(status);
    }
}

void add_to_queue() {
    if (current_column == 2) {
        mpd_run_add(conn, songs.items[songs.selected].uri);
    }
}

void clear_queue() {
    mpd_run_clear(conn);
}

void show_help() {
    WINDOW *help_win = newwin(LINES, COLS, 0, 0);
    keypad(help_win, TRUE);
    
    box(help_win, 0, 0);
    
    mvwprintw(help_win, 1, (COLS - 20) / 2, "Sheep MPD Client Help");
    mvwhline(help_win, 2, 1, ACS_HLINE, COLS - 2);
    
    char *help_text[] = {
        "Navigation:",
        "  h/l or Left/Right - Switch columns",
        "  k/j or Up/Down - Move selection",
        "  Enter - Select item / Play song",
        "",
        "Playback Control:",
        "  Space - Play/Pause",
        "  < / > - Previous / Next track",
        "  + / - - Volume up / down",
        "  f / b - Seek forward / backward",
        "  0-9 - Jump to percentage of song",
        "",
        "Queue Management:",
        "  a - Add selected song to queue",
        "  C - Clear queue",
        "",
        "Other Commands:",
        "  r/z/s/c - Toggle Repeat/Random/Single/Consume",
        "  / - Search",
        "  R - Refresh library",
        "  q - Quit",
        "",
        "Press any key to return"
    };
    
    int start_y = 4;
    for (size_t i = 0; i < sizeof(help_text) / sizeof(help_text[0]); i++) {
        mvwprintw(help_win, start_y + i, 2, "%s", help_text[i]);
    }
    
    wrefresh(help_win);
    
    nodelay(help_win, FALSE);
    wgetch(help_win);
    
    delwin(help_win);
}

void toggle_playback_mode(int mode) {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        switch (mode) {
            case SHEEP_KEY_TOGGLE_REPEAT:
                mpd_run_repeat(conn, !mpd_status_get_repeat(status));
                break;
            case SHEEP_KEY_TOGGLE_RANDOM:
                mpd_run_random(conn, !mpd_status_get_random(status));
                break;
            case SHEEP_KEY_TOGGLE_SINGLE:
                mpd_run_single(conn, !mpd_status_get_single(status));
                break;
            case SHEEP_KEY_TOGGLE_CONSUME:
                mpd_run_consume(conn, !mpd_status_get_consume(status));
                break;
        }
        mpd_status_free(status);
    }
}

void search(const char *term) {
    Column *col;
    switch (current_column) {
        case 0: col = &artists; break;
        case 1: col = &albums; break;
        case 2: col = &songs; break;
        default: return;
    }
    
    for (int i = 0; i < col->count; i++) {
        if (strcasestr(col->items[i].name, term)) {
            col->selected = i;
            col->scroll_offset = (i > LINES / 2) ? i - LINES / 2 : 0;
            break;
        }
    }
}

int time_elapsed(struct timeval *last_time) {
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    return (current_time.tv_sec - last_time->tv_sec) * 1000000 + (current_time.tv_usec - last_time->tv_usec);
}

void seek(int direction) {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        unsigned int current_pos = mpd_status_get_elapsed_time(status);
        unsigned int total_time = mpd_status_get_total_time(status);
        int new_pos = current_pos + direction * SEEK_STEP;
        if (new_pos < 0) new_pos = 0;
        if ((unsigned int)new_pos > total_time) new_pos = total_time;
        mpd_run_seek_pos(conn, mpd_status_get_song_pos(status), new_pos);
        mpd_status_free(status);
    }
}

void jump_to_percentage(int percentage) {
    struct mpd_status *status = mpd_run_status(conn);
    if (status) {
        unsigned int total_time = mpd_status_get_total_time(status);
        unsigned int new_pos = (total_time * percentage) / 100;
        mpd_run_seek_pos(conn, mpd_status_get_song_pos(status), new_pos);
        mpd_status_free(status);
    }
}

int main() {
    setlocale(LC_ALL, "");
    init_mpd();
    fetch_artists();
    
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    start_color();
    use_default_colors();
    curs_set(0);
    timeout(100);  // Set getch() timeout to 100ms
    
    init_pair(1, COLOR_FOREGROUND, COLOR_BACKGROUND);
    init_pair(2, COLOR_SELECTION, COLOR_BACKGROUND);
    init_pair(3, COLOR_ACTIVE, COLOR_BACKGROUND);
    init_pair(4, COLOR_INACTIVE, COLOR_BACKGROUND);
    init_pair(5, COLOR_HIGHLIGHT, COLOR_BACKGROUND);
    init_pair(6, COLOR_STATUS, COLOR_BACKGROUND);
    init_pair(7, COLOR_SEARCH, COLOR_BACKGROUND);
    init_pair(8, COLOR_ERROR, COLOR_BACKGROUND);
    init_pair(9, COLOR_PLAYING, COLOR_BACKGROUND);
    
    int max_y, max_x;
    getmaxyx(stdscr, max_y, max_x);
    main_win = newwin(max_y - 3, max_x, 0, 0);
    status_win = newwin(3, max_x, max_y - 3, 0);
    
    gettimeofday(&last_update_time, NULL);
    gettimeofday(&last_search_time, NULL);
    
    // Initialize albums and songs for the first artist
    if (artists.count > 0) {
        fetch_albums(artists.items[0].name);
        if (albums.count > 0) {
            fetch_songs(artists.items[0].name, albums.items[0].name);
        }
    }
    
    int ch;
    bool running = true;
    
    while (running) {
        draw_screen();
        ch = getch();
        
        if (ch == ERR) {
            // No input, continue to next iteration
            continue;
        }
        
        if (search_mode) {
            if (ch == '\n') {
                search_mode = 0;
                search(search_term);
            } else if (ch == 27) { // ESC key
                search_mode = 0;
                search_term[0] = '\0';
            } else if (ch == KEY_BACKSPACE || ch == 127) {
                if (strlen(search_term) > 0) {
                    search_term[strlen(search_term) - 1] = '\0';
                }
            } else if (isprint(ch)) {
                if (strlen(search_term) < MAX_LENGTH - 1) {
                    search_term[strlen(search_term)] = ch;
                    search_term[strlen(search_term) + 1] = '\0';
                }
            }
            if (time_elapsed(&last_search_time) > SEARCH_DELAY) {
                search(search_term);
                gettimeofday(&last_search_time, NULL);
            }
        } else {
            switch (ch) {
                case SHEEP_KEY_QUIT:
                    running = false;
                    break;
                case SHEEP_KEY_UP:
                case KEY_UP:
                    if (current_column == 0 && artists.selected > 0) {
                        artists.selected--;
                        fetch_albums(artists.items[artists.selected].name);
                        albums.selected = 0;
                        fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        songs.selected = 0;
                    } else if (current_column == 1 && albums.selected > 0) {
                        albums.selected--;
                        fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        songs.selected = 0;
                    } else if (current_column == 2 && songs.selected > 0) {
                        songs.selected--;
                    }
                    break;
                case SHEEP_KEY_DOWN:
                case KEY_DOWN:
                    if (current_column == 0 && artists.selected < artists.count - 1) {
                        artists.selected++;
                        fetch_albums(artists.items[artists.selected].name);
                        albums.selected = 0;
                        fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        songs.selected = 0;
                    } else if (current_column == 1 && albums.selected < albums.count - 1) {
                        albums.selected++;
                        fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        songs.selected = 0;
                    } else if (current_column == 2 && songs.selected < songs.count - 1) {
                        songs.selected++;
                    }
                    break;
                case SHEEP_KEY_LEFT:
                case KEY_LEFT:
                    if (current_column > 0) {
                        current_column--;
                    }
                    break;
                case SHEEP_KEY_RIGHT:
                case KEY_RIGHT:
                    if (current_column < 2) {
                        current_column++;
                        if (current_column == 1) {
                            fetch_albums(artists.items[artists.selected].name);
                        } else if (current_column == 2) {
                            fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        }
                    }
                    break;
                case SHEEP_KEY_ENTER:
                case KEY_ENTER:
                    if (current_column == 2) {
                        play_selected_song();
                    } else if (current_column < 2) {
                        current_column++;
                        if (current_column == 1) {
                            fetch_albums(artists.items[artists.selected].name);
                        } else if (current_column == 2) {
                            fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                        }
                    }
                    break;
                case SHEEP_KEY_PLAY_PAUSE:
                    toggle_playback();
                    break;
                case SHEEP_KEY_NEXT_TRACK:
                    mpd_run_next(conn);
                    break;
                case SHEEP_KEY_PREV_TRACK:
                    mpd_run_previous(conn);
                    break;
                case SHEEP_KEY_VOLUME_UP:
                    change_volume(VOLUME_STEP);
                    break;
                case SHEEP_KEY_VOLUME_DOWN:
                    change_volume(-VOLUME_STEP);
                    break;
                case SHEEP_KEY_TOGGLE_REPEAT:
                case SHEEP_KEY_TOGGLE_RANDOM:
                case SHEEP_KEY_TOGGLE_SINGLE:
                case SHEEP_KEY_TOGGLE_CONSUME:
                    toggle_playback_mode(ch);
                    break;
                case SHEEP_KEY_ADD_TO_QUEUE:
                    add_to_queue();
                    break;
                case SHEEP_KEY_CLEAR_QUEUE:
                    clear_queue();
                    break;
                case SHEEP_KEY_HELP:
                    show_help();
                    clear();
                    refresh();
                    break;
                case SHEEP_KEY_SEARCH:
                    search_mode = 1;
                    search_term[0] = '\0';
                    break;
                case SHEEP_KEY_REFRESH:
                    fetch_artists();
                    fetch_albums(artists.items[artists.selected].name);
                    fetch_songs(artists.items[artists.selected].name, albums.items[albums.selected].name);
                    break;
                case SHEEP_KEY_SEEK_FORWARD:
                    seek(1);
                    break;
                case SHEEP_KEY_SEEK_BACKWARD:
                    seek(-1);
                    break;
                case SHEEP_KEY_JUMP_10_PERCENT:
                case SHEEP_KEY_JUMP_20_PERCENT:
                case SHEEP_KEY_JUMP_30_PERCENT:
                case SHEEP_KEY_JUMP_40_PERCENT:
                case SHEEP_KEY_JUMP_50_PERCENT:
                case SHEEP_KEY_JUMP_60_PERCENT:
                case SHEEP_KEY_JUMP_70_PERCENT:
                case SHEEP_KEY_JUMP_80_PERCENT:
                case SHEEP_KEY_JUMP_90_PERCENT:
                case SHEEP_KEY_JUMP_100_PERCENT:
                    jump_to_percentage((ch - '0') * 10);
                    break;
                case KEY_RESIZE:
                    clear();
                    refresh();
                    getmaxyx(stdscr, max_y, max_x);
                    wresize(main_win, max_y - 3, max_x);
                    wresize(status_win, 3, max_x);
                    mvwin(status_win, max_y - 3, 0);
                    break;
            }
        }
    }
    
    endwin();
    mpd_connection_free(conn);
    return 0;
}
