#ifndef CONFIG_H
#define CONFIG_H

/* Appearance */
#define COLOR_BACKGROUND   COLOR_BLACK
#define COLOR_FOREGROUND   COLOR_WHITE
#define COLOR_SELECTION    COLOR_CYAN
#define COLOR_ACTIVE       COLOR_BLUE
#define COLOR_INACTIVE     COLOR_WHITE
#define COLOR_HIGHLIGHT    COLOR_YELLOW
#define COLOR_STATUS       COLOR_GREEN
#define COLOR_SEARCH       COLOR_MAGENTA
#define COLOR_ERROR        COLOR_RED
#define COLOR_PLAYING      COLOR_GREEN

/* MPD connection settings */
#define MPD_HOST "localhost"
#define MPD_PORT 6600
#define MPD_TIMEOUT 30000

/* Keybindings */
#define SHEEP_KEY_QUIT        'q'
#define SHEEP_KEY_UP          'k'
#define SHEEP_KEY_DOWN        'j'
#define SHEEP_KEY_LEFT        'h'
#define SHEEP_KEY_RIGHT       'l'
#define SHEEP_KEY_ENTER       '\n'
#define SHEEP_KEY_PLAY_PAUSE  ' '
#define SHEEP_KEY_NEXT_TRACK  '>'
#define SHEEP_KEY_PREV_TRACK  '<'
#define SHEEP_KEY_VOLUME_UP   '+'
#define SHEEP_KEY_VOLUME_DOWN '-'
#define SHEEP_KEY_TOGGLE_REPEAT 'r'
#define SHEEP_KEY_TOGGLE_RANDOM 'z'
#define SHEEP_KEY_TOGGLE_SINGLE 's'
#define SHEEP_KEY_TOGGLE_CONSUME 'c'
#define SHEEP_KEY_ADD_TO_QUEUE 'a'
#define SHEEP_KEY_CLEAR_QUEUE 'C'
#define SHEEP_KEY_HELP        '?'
#define SHEEP_KEY_SEARCH      '/'
#define SHEEP_KEY_REFRESH     'R'
#define SHEEP_KEY_SEEK_FORWARD 'f'
#define SHEEP_KEY_SEEK_BACKWARD 'b'
#define SHEEP_KEY_JUMP_10_PERCENT '1'
#define SHEEP_KEY_JUMP_20_PERCENT '2'
#define SHEEP_KEY_JUMP_30_PERCENT '3'
#define SHEEP_KEY_JUMP_40_PERCENT '4'
#define SHEEP_KEY_JUMP_50_PERCENT '5'
#define SHEEP_KEY_JUMP_60_PERCENT '6'
#define SHEEP_KEY_JUMP_70_PERCENT '7'
#define SHEEP_KEY_JUMP_80_PERCENT '8'
#define SHEEP_KEY_JUMP_90_PERCENT '9'
#define SHEEP_KEY_JUMP_100_PERCENT '0'

/* Playback settings */
#define VOLUME_STEP 5
#define SEEK_STEP 10

/* UI settings */
#define SHOW_PROGRESS_BAR 1
#define SHOW_VOLUME 1
#define SHOW_PLAYBACK_MODE 1
#define SHOW_TIME_ELAPSED 1
#define SHOW_TIME_TOTAL 1
#define SHOW_BITRATE 1
#define SHOW_SAMPLE_RATE 1

/* Progress bar settings */
#define PROGRESS_BAR_CHAR '='
#define PROGRESS_BAR_BG '-'
#define PROGRESS_BAR_CURSOR '>'

/* Column widths (percentage of screen width) */
#define ARTIST_COLUMN_WIDTH 30
#define ALBUM_COLUMN_WIDTH 30
#define SONG_COLUMN_WIDTH 40

/* Maximum number of items to display in each column */
#define MAX_ITEMS 10000

/* Maximum length of strings */
#define MAX_LENGTH 256

/* Auto-refresh delay (in microseconds) */
#define AUTO_REFRESH_DELAY 100000

/* Search settings */
#define SEARCH_DELAY 300000

/* Scrolling settings */
#define SCROLL_OFFSET 5

/* Error display duration (in seconds) */
#define ERROR_DISPLAY_DURATION 3

#endif // CONFIG_H
