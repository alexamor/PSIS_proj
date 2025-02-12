#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int dim_board;
board_place * board;
//int play1[2];
int n_corrects;
play_response resp[MAX_PLAYERS];
pthread_rwlock_t locks[MAX_PLAYERS] = PTHREAD_RWLOCK_INITIALIZER; // locks for synchronization


int linear_conv(int i, int j){
  return j*dim_board+i;
}

/*This function returns a pointer to the string of card referenced by the coordinates (I,j)*/
char * get_board_place_str(int i, int j){
  return board[linear_conv(i, j)].v;
}


/*Creates a board with dim*dim dimension and assigns each card a string.*/
void init_board(int dim){
  int count  = 0;
  int i, j;
  char * str_place;


  /*Initialization of global variables*/
  dim_board= dim; /*Initializes board dimension*/
  n_corrects = 0;
  board = malloc(sizeof(board_place)* dim *dim);

  /*Board is an array of board_places. Each board place is an array of 3 chars ('aa\0'). */
  /*Intializes each board_place with '\0'*/
  for( i=0; i < (dim_board*dim_board); i++){
	board[i].v[0] = '\0';
	board[i].state = 0;
  }
  for( i = 0; i < MAX_PLAYERS; i++){
  	resp[i].play1[0] = -1;
  }  

  /*Fills each board_place randomly*/
  for (char c1 = 'a' ; c1 < ('a'+dim_board); c1++){
	for (char c2 = 'a' ; c2 < ('a'+dim_board); c2++){
		/*Tries to find, randomly, a place that's still empty*/
		do{
			i = random()% dim_board;
			j = random()% dim_board;
			str_place = get_board_place_str(i, j);
			printf("%d %d -%s-\n", i, j, str_place); /*str_place[0] = board_place[].v[0]*/
		}while(str_place[0] != '\0');
		
		/*Fills the place that it found*/
		str_place[0] = c1;
		str_place[1] = c2;
		str_place[2] = '\0';
		
		/*Tries to find a second random place that's still empty*/
		do{
			i = random()% dim_board;
			j = random()% dim_board;
			str_place = get_board_place_str(i, j);
			printf("%d %d -%s-\n", i, j, str_place);
		}while(str_place[0] != '\0');

		/*Fills the place that it found*/
		str_place[0] = c1;
		str_place[1] = c2;
		str_place[2] = '\0';

		/*Number of places filled is incremented by 2*/
		count += 2;

		/*Checks if the board is full*/
		if (count == dim_board*dim_board)
			return;
		}
  }
}

/*Receives the coordinates of a card to be picked and, depending of the card state (DOWN, UP or LOCKED), updates it and returns suitable information*/
play_response board_play(int p, int id){
	int x, y;
	int play1[2];
	play_response aux_play;
	/*Initialization of variables*/
	
	//Lock to write
	pthread_rwlock_wrlock(&(locks[id]));
	resp[id].code = 10;
	pthread_rwlock_unlock(&(locks[id]));

	x = get_x(p);
	y = get_y(p);

	board_place aux_place = get_board_place(p);

	/*If the place chosen is already filled - chosen or already correct*/
	if(aux_place.state == 3 || aux_place.state == 1 ){
		//printf("FILLED\n");
		pthread_rwlock_wrlock(&(locks[id]));
		resp[id].code = 0;
		pthread_rwlock_unlock(&(locks[id]));


		aux_play = resp[id];
	}else{
		/*Checks if it's the first card chosen*/
		pthread_rwlock_wrlock(&(locks[id]));
		if((resp[id].play1[0] == -1) ){
			printf("FIRST\n");
			resp[id].code = 1;

			/*Saves first card chosen*/
			play1[0] = x;
			play1[1] = y;
			resp[id].play1[0] = play1[0];
			resp[id].play1[1] = play1[1];
			strcpy(resp[id].str_play1, get_board_place_str(x, y));

			aux_play = resp[id];
		}
		/*If it's the second card chosen*/
		else{
			/*Gets first card chosen*/
			char * first_str = get_board_place_str(resp[id].play1[0], resp[id].play1[1]);
			char * secnd_str = get_board_place_str(x, y);

			/*Checks if the card chosen is the same as the first one chosen*/
			if ((resp[id].play1[0]==x) && (resp[id].play1[1]==y)){
				resp[id].code =0;
				//printf("FILLED\n");
				aux_play = resp[id];
			}
			/*Compares both plays*/
			else{
				/*resp[id].play1[0]= play1[0];
				resp[id].play1[1]= play1[1];*/
				strcpy(resp[id].str_play1, first_str);
				resp[id].play2[0]= x;
				resp[id].play2[1]= y;
				strcpy(resp[id].str_play2, secnd_str);

				/*Compares strings of each play - if strings match*/
				if (strcmp(first_str, secnd_str) == 0){
					printf("CORRECT!!!\n");


					//strcpy(first_str, "");
					//strcpy(secnd_str, "");

					/*Increments number of correct plays*/
					n_corrects +=2;
					/*Checks if the game as ended and send message END or CORRECT PLAY*/
					if (n_corrects == dim_board* dim_board)
						resp[id].code =3;
					else
						resp[id].code =2;
				}
				/*Strings don't match*/
				else{
					printf("INCORRECT");

					resp[id].code = -2;
				}
				aux_play = resp[id];

				resp[id].play1[0]= -1;
			}
		}
		pthread_rwlock_unlock(&(locks[id]));
	}

	return aux_play;
}

void get_colors(color color_players[]){
	int i;

	for (i = 0; i < MAX_PLAYERS; i++){
		color_players[i] = get_single_color(i); 
	}

}

color get_single_color( int i){

	color color_players;

	color_players.r = (i%3)*100 + 50;
	color_players.g = 255 - ((i%4)*70 + 40);
	color_players.b = (i%5)*50 + 50;

	return color_players;
}

void clear_memory(){
	free(board);
}

void update_board_place(int board_x, int board_y, int player, int code){
	
	board[linear_conv(board_x, board_y)].player = player;
	board[linear_conv(board_x, board_y)].state = code;

}


board_place* get_board(){
	return board;
}

int get_x(int p){
	int x = (int) p%dim_board;
	//printf("x : %d\n", x);
	return x;
}

int get_y(int p){
	int y = p/dim_board;
	//printf("y : %d\n", y);
	return y;
}

board_place get_board_place(int p){

	return board[p];

}

bool if_card_chosen(int x, int y){
	int i;
	for(i = 0; i < MAX_PLAYERS; i++){
		
		pthread_rwlock_rdlock(&(locks[i]));
		if((resp[i].play1[0] == x) && (resp[i].play1[1] == y)){
			pthread_rwlock_unlock(&(locks[i]));
			return true;
		}
		pthread_rwlock_unlock(&(locks[i]));
		
	}
	return false;
}

play last_played(int id){
	play aux_play;
	int aux_p;
	board_place aux_place;


	aux_p = linear_conv(resp[id].play1[0], resp[id].play1[1]);


	board[aux_p].state = 0;


	aux_place = get_board_place(aux_p);
	aux_play.place = aux_place;
	aux_play.x = resp[id].play1[0];
	aux_play.y = resp[id].play1[1];

	resp[id].play1[0] = -1;

	return aux_play;
}