#include <stdlib.h>
#include "board_library.h"
#include <stdio.h>
#include <string.h>
#include <pthread.h>

int dim_board;
board_place * board;
//int play1[2];
int n_corrects;
//play_response resp[MAX_PLAYERS];
//pthread_rwlock_t locks[MAX_PLAYERS] = PTHREAD_RWLOCK_INITIALIZER; // locks for synchronization
play_node *head;


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

  /*Fills each board_place randomly*/
  for (char c1 = 'a' ; c1 < ('a'+dim_board); c1++){
	for (char c2 = 'a' ; c2 < ('a'+dim_board); c2++){
		/*Tries to find, randomly, a place that's still empty*/
		do{
			i = random()% dim_board;
			j = random()% dim_board;
			str_place = get_board_place_str(i, j);
			printf("spawn %d %d -%s-\n", i, j, str_place); /*str_place[0] = board_place[].v[0]*/
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
			printf("spawn %d %d -%s-\n", i, j, str_place);
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
	play_node *aux;
	/*Initialization of variables*/

	aux = get_play_node(id);
	
	//Lock to write
	pthread_rwlock_wrlock(&(aux->lock));
	aux->resp.code = 10;
	pthread_rwlock_unlock(&(aux->lock));

	x = get_x(p);
	y = get_y(p);

	board_place aux_place = get_board_place(p);

	/*If the place chosen is already filled - chosen or already correct*/
	if(aux_place.state == 3 || aux_place.state == 1 ){
		//printf("FILLED\n");
		pthread_rwlock_wrlock(&(aux->lock));
		aux->resp.code = 0;
		pthread_rwlock_unlock(&(aux->lock));


		aux_play = aux->resp;
	}else{
		/*Checks if it's the first card chosen*/
		pthread_rwlock_wrlock(&(aux->lock));
		if((aux->resp.play1[0] == -1) ){
			printf("FIRST\n");
			aux->resp.code = 1;

			/*Saves first card chosen*/
			play1[0] = x;
			play1[1] = y;
			aux->resp.play1[0] = play1[0];
			aux->resp.play1[1] = play1[1];
			strcpy(aux->resp.str_play1, get_board_place_str(x, y));

			aux_play = aux->resp;
		}
		/*If it's the second card chosen*/
		else{
			/*Gets first card chosen*/
			char * first_str = get_board_place_str(aux->resp.play1[0], aux->resp.play1[1]);
			char * secnd_str = get_board_place_str(x, y);

			/*Checks if the card chosen is the same as the first one chosen*/
			if ((aux->resp.play1[0]==x) && (aux->resp.play1[1]==y)){
				aux->resp.code =0;
				//printf("FILLED\n");
				aux_play = aux->resp;
			}
			/*Compares both plays*/
			else{
				/*resp[id].play1[0]= play1[0];
				resp[id].play1[1]= play1[1];*/
				strcpy(aux->resp.str_play1, first_str);
				aux->resp.play2[0]= x;
				aux->resp.play2[1]= y;
				strcpy(aux->resp.str_play2, secnd_str);

				/*Compares strings of each play - if strings match*/
				if (strcmp(first_str, secnd_str) == 0){
					printf("CORRECT!!!\n");


					//strcpy(first_str, "");
					//strcpy(secnd_str, "");

					/*Increments number of correct plays*/
					n_corrects +=2;
					/*Checks if the game as ended and send message END or CORRECT PLAY*/
					if (n_corrects == dim_board* dim_board)
						aux->resp.code =3;
					else
						aux->resp.code =2;
				}
				/*Strings don't match*/
				else{
					printf("INCORRECT\n");

					aux->resp.code = -2;
				}
				aux_play = aux->resp;

				aux->resp.play1[0]= -1;
			}
		}
		pthread_rwlock_unlock(&(aux->lock));
	}

	return aux_play;
}

/*Returns color of the player given by argument*/
color get_single_color( int i){

	color color_players;

	color_players.r = (i%3)*100 + 50;
	color_players.g = 255 - ((i%4)*70 + 40);
	color_players.b = (i%5)*50 + 50;

	return color_players;
}

/*Frees memory*/
void clear_memory(){
	free(board);
}

/*Updates state and player of the card*/
void update_board_place(int board_x, int board_y, int player, int code){
	
	board[linear_conv(board_x, board_y)].player = player;
	board[linear_conv(board_x, board_y)].state = code;

}

/*Returns the board pointer.*/
board_place* get_board(){
	return board;
}

/*Returns coordinate x given the index of the array*/
int get_x(int p){
	int x = (int) p%dim_board;
	//printf("x : %d\n", x);
	return x;
}

/*Returns coordinate y given the index of the array*/
int get_y(int p){
	int y = p/dim_board;
	//printf("y : %d\n", y);
	return y;
}

/*Returns coordinate y given the index of the array*/
board_place get_board_place(int p){

	return board[p];

}

/*Checks if the card has already been chosen*/
bool if_card_chosen(int x, int y){
	play_node *aux;


	for(aux = head->next; aux!= NULL; aux = aux->next){
		
		pthread_rwlock_rdlock(&(aux->lock));
		if((aux->resp.play1[0] == x) && (aux->resp.play1[1] == y)){
			pthread_rwlock_unlock(&(aux->lock));
			return true;
		}
		pthread_rwlock_unlock(&(aux->lock));
		
	}
	return false;
}

/*When the timer reachers 5 seconds, resets the play and updates the state of the card to turn it white*/
play last_played(int id){
	play aux_play;
	int aux_p;
	board_place aux_place;
	play_node *aux;

	aux = get_play_node(id);

	/*Gets first play*/
	aux_p = linear_conv(aux->resp.play1[0], aux->resp.play1[1]);

	/*Updates state to turn it white again*/
	board[aux_p].state = 0;


	aux_place = get_board_place(aux_p);
	aux_play.place = aux_place;
	aux_play.x = aux->resp.play1[0];
	aux_play.y = aux->resp.play1[1];

	/*Resets 1st play*/
	aux->resp.play1[0] = -1;

	return aux_play;
}

/*Initializes list of players*/
void init_head(){
	head = malloc(sizeof(play_node));
	head->id = -1;
	head->next = NULL;
}

//Feito com empty head, a adição é feita sempre após esta
play_node* add_play(int id){

	play_node* new;

	new = malloc(sizeof(play_node));

	new->id = id;
	new->resp.play1[0] = -1;
	pthread_rwlock_init(&(new->lock), NULL);
	new->next = head->next;
	head->next = new;

	return new;
}

/*Removes player from list*/
void remove_play(int id){

	play_node *aux, *removed;

	for(aux = head; aux!= NULL; aux = aux->next){
		if(aux->next->id == id){
			removed = aux->next;
			aux->next = aux->next->next;
			free(removed);
			break;
		}
	}
}

/*Returns player from list given its id*/
play_node* get_play_node(int id){

	play_node *aux;

	for(aux = head; aux != NULL; aux = aux->next){
		if(aux->id == id){
			return aux;
		}
	}

	return NULL;
}

/*frees list*/
void clear_plays(){
	play_node * aux, *aux2;
	
	for(aux = head; aux != NULL;){
		aux2 = aux;
		aux = aux->next;
		free(aux2);
	}
}
