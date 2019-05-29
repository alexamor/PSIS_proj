#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "board_library.h" 
#include "UI_library.h"

#define PORT 3000

typedef struct card_node{
	int id;
	int toTurnCards[2][2];
	char prev_card[3];
	struct card_node *next;
}card_node;

bool done = false;
int board_size;
board_place * board;
card_node *head;

void* checkForPlays(void* args);
void draw_board();
void init_card_head();
card_node* add_cards(int id);
void remove_cards(int id);
card_node* get_card_node(int id);
void clear_cards();

int main(int argc, char * argv[]){
	struct sockaddr_in server_addr;
	play read_play;
	color player_color;
	pthread_t SDL_thread;
	int sock_fd;
	int red_cards = 0;
	int i, j;
	card_node *aux;

	

	if (argc <2){
	    printf("second argument should be server address\n");
	    exit(-1);
	}
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	if (sock_fd == -1){
	    perror("socket: ");
		exit(-1);
	}
	server_addr.sin_family = AF_INET;
	// this values can be read from the keyboard
	server_addr.sin_port= htons(PORT);
	inet_aton(argv[1], &server_addr.sin_addr);

    if( -1 == connect(sock_fd, (const struct sockaddr *) &server_addr, sizeof(server_addr))){
		printf("Error connecting\n");
		exit(-1);
	}

  	printf("1 - client connected\n");

  	init_card_head();


    read(sock_fd, &board_size, sizeof(board_size));
    printf("board_size %d\n", board_size);
    
	/*Initializes SDL*/
	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
		 printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
		 exit(-1);
	}
	if(TTF_Init()==-1) {
			printf("TTF_Init: %s\n", TTF_GetError());
			exit(2);
	}
	
    create_board_window(300, 300, board_size);

    board = (board_place *) malloc(sizeof(board_place)* board_size * board_size);

    printf("Board allocated\n");

    read(sock_fd, board, sizeof(board_place) * board_size * board_size);

    printf("Board read\n");

    draw_board();

    printf("Board draw\n");
	
	//threads here
	pthread_create(&SDL_thread, NULL, checkForPlays, (void*) &sock_fd);

	printf("thread checkForPlays created\n");


	while(done == false && read(sock_fd, &read_play, sizeof(read_play)) != 0){
		if(1){

			printf("Play read\n");

			player_color = get_single_color(read_play.place.player);
			aux = get_card_node(read_play.place.player);

			printf("Card: %d %d   State: %d\n ", read_play.x, read_play.y, read_play.place.state );

			//renders the card given the type of play
			switch (read_play.place.state) {
				//turn card down
				case 0:
					paint_card(read_play.x, read_play.y , 255, 255, 255);
					//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);
					break;
				//turn up and letters grey
				case 1:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, read_play.place.v, 200, 200, 200);
					aux->toTurnCards[0][0] = read_play.x;
					aux->toTurnCards[0][1] = read_play.y;
					strcpy(aux->prev_card, read_play.place.v);
					break;
				//wrong card up - letters red
				case 2:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, read_play.place.v, 255, 0, 0);
					write_card(aux->toTurnCards[0][0], aux->toTurnCards[0][1], aux->prev_card, 255, 0, 0);
					red_cards++;
					aux->toTurnCards[1][0] = read_play.x;
					aux->toTurnCards[1][1] = read_play.y;
					break;
				//locked card - letters black
				case 3:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, aux->prev_card, 0, 0, 0);
					write_card(aux->toTurnCards[0][0], aux->toTurnCards[0][1], aux->prev_card, 0, 0, 0);
					break;
				//win - board completed
				case 5:
				case 4:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, aux->prev_card, 0, 0, 0);
					write_card(aux->toTurnCards[0][0], aux->toTurnCards[0][1], aux->prev_card, 0, 0, 0);
					
					//Print end game prompt
					if(read_play.place.state == 4)
						end_prompt(board_size, false);
					else
						end_prompt(board_size, true);

					//done = true;
					sleep(10);
					for(i = 0; i < board_size; i++){
						for(j = 0; j < board_size; j++){
							paint_card(i, j , 255, 255, 255);
						}
					}
					break;	

			}

			if(red_cards == 1){
				sleep(2);
				paint_card(aux->toTurnCards[0][0], aux->toTurnCards[0][1], 255, 255, 255);
				paint_card(aux->toTurnCards[1][0], aux->toTurnCards[1][1], 255, 255, 255);
				red_cards = 0;
			}


		}
	}

	pthread_join(SDL_thread, NULL);

  	close(sock_fd);

  	clear_cards();

  	return 0;

}


void* checkForPlays( void* args){

	SDL_Event event;
	int p;

	int* aux = args;
	int sock_fd = *aux;


	/*While the game isn't over*/
	while (!done){
		// Event is a click of the mouse
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
				// if someone clics on the x, game ends
				case SDL_QUIT: {
					done = true;
					break;
				}
				// if someone clicks on the board, it's a play
				case SDL_MOUSEBUTTONDOWN:{
					int board_x, board_y;
					// get place where the mouse clicked
					get_board_card(event.button.x, event.button.y, &board_x, &board_y);

					printf("click (%d %d) -> (%d %d)\n", event.button.x, event.button.y, board_x, board_y);

					p = board_x + board_size * board_y;

					printf("p %d\n", p);

					//Send play to server
					write(sock_fd, &p, sizeof(p));

					break;
				}
			}
		}
	}

	printf("fim\n");
	close_board_windows();
}

void draw_board(){
	int x, y;
	color player_color;

	for(int i = 0; i < (board_size * board_size); i++){

		printf("i %d  code %d\n", i, board[i].state);

		x = (int) i%board_size;
		y = (int) i/board_size; 

		player_color = get_single_color(board[i].player);


		//printf("Card: %d %d   State: %d\n ", x , y, board[i].state );

		//renders the card given the type of play
		switch (board[i].state) {
			//turn card down
			case 0:
				paint_card(x, y, 255, 255, 255);
				//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);
				break;
			//turn up and letters grey
			case 1:
				paint_card(x, y, player_color.r, player_color.g, player_color.b);
				write_card(x, y, board[i].v, 200, 200, 200);
				break;
			//wrong card up - letters red
			case 2:
				paint_card(x, y, player_color.r, player_color.g, player_color.b);
				write_card(x, y, board[i].v, 255, 0, 0);
				break;
			//locked card - letters black
			case 3:
				paint_card(x, y, player_color.r, player_color.g, player_color.b);
				write_card(x, y, board[i].v, 0, 0, 0);
				break;	

		}
	}
}

void init_card_head(){
	head = malloc(sizeof(card_node));
	head->id = -1;
	head->next = NULL;
}

//Feito com empty head, a adição é feita sempre após esta
card_node* add_cards(int id){

	card_node* new;

	new = malloc(sizeof(card_node));

	new->id = id;
	new->next = head->next;
	head->next = new;

	return new;
}

void remove_cards(int id){

	card_node *aux, *removed;

	for(aux = head; aux!= NULL; aux = aux->next){
		if(aux->next->id == id){
			removed = aux->next;
			aux->next = aux->next->next;
			free(removed);
			break;
		}
	}
}

card_node* get_card_node(int id){

	card_node *aux;

	for(aux = head; aux != NULL; aux = aux->next){
		if(aux->id == id){
			return aux;
		}
	}

	//caso nao haja cria-se
	aux = add_cards(id);

	return aux;
}

void clear_cards(){
	card_node * aux, *aux2;

	free(board);
	for(aux = head; aux != NULL;){
		aux2 = aux;
		aux = aux->next;
		free(aux2);
	}
}

