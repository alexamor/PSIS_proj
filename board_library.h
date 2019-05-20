#include <stdlib.h>

#define MAX_PLAYERS 8

typedef struct board_place{
  char v[3];
  int player;
  int state;
  //0 - down (no letters)
  //1 - up - first play (grey)
  //2 - up - wrong (red)
  //3 - locked (black)
  //4 - win
} board_place;

typedef struct play{
	board_place place;
	int x;
	int y;
} play;

typedef struct play_response{
  int code; // 0 - filled
            // 1 - 1st play
            // 2 2nd - same plays
            // 3 END
            // -2 2nd - diffrent
  int play1[2];
  int play2[2];
  char str_play1[3], str_play2[3];
} play_response;

typedef struct color{
	int r;
	int g;
	int b;
}color;


char * get_board_place_str(int i, int j);
void init_board(int dim);
play_response board_play (int x, int y);
void get_colors(color color_players[]);
color get_single_color(int i);
void clear_memory();
void update_board_place(int board_x, int board_y, int player, int code);
