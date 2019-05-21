#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdbool.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>

#include "board_library.h" 
#include "UI_library.h"

#define PORT 3000
#define INACTIVE -2


color color_players[MAX_PLAYERS];
int sock_fd;
int players_fd[MAX_PLAYERS];
int nr_active_players = 0;
bool done = false;
struct sockaddr_in local_addr;
int dim;

//void* process_events();
void* process_players(void* args);
void* accept_players();


int main(int argc, char* argv[]){
	// server variables
	int i;
	pthread_t SDL_thread;
	pthread_t get_players;
	
	if(argc < 2){
		printf("Please provide the dimension of the board as an argument.\n");
		exit(1);
	}

	dim = atoi(argv[1]);
	if( (dim%2 != 0) || (dim < 1)) {
		printf("Please provide a multiple of 2 as an argument.\n");
		exit(1);
	}

	/*Get colors of each player*/
	get_colors(color_players);

	/*Creates thread to deal with processes*/
	//pthread_create(&SDL_thread, NULL, process_events, NULL);

	/*Create socket*/
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
		perror("socket: ");
	    exit(-1);
	}

	local_addr.sin_family = AF_INET;
	local_addr.sin_port= htons(PORT);
	local_addr.sin_addr.s_addr= INADDR_ANY;

	/*Bind socket to local address*/
	if( bind(sock_fd, (struct sockaddr *)&local_addr, sizeof(local_addr)) == -1) {
	    perror("bind");
	    exit(-1);
	}

	printf("Socket created and binded \n");
	/*Start listening on the socket*/
	if(listen(sock_fd, 5) == -1){
		perror("listen");
		exit(-1);
	}

	for(i = 0; i < MAX_PLAYERS; i++){
		players_fd[i] = INACTIVE;
	}

	printf("Waiting for players...\n");

	pthread_create(&(get_players), NULL, accept_players, NULL);





	//pthread_join(SDL_thread, NULL);

	printf("fim\n");
	close(sock_fd);
	clear_memory();
}


// void* process_events(){
// 	SDL_Event event; //Creates event

// 	/*Initializes SDL*/
// 	if( SDL_Init( SDL_INIT_VIDEO ) < 0 ) {
// 		 printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
// 		 exit(-1);
// 	}
// 	if(TTF_Init()==-1) {
// 			printf("TTF_Init: %s\n", TTF_GetError());
// 			exit(2);
// 	}

// 	/*Renders window*/
// 	create_board_window(300, 300, dim);
// 	/*Fills board*/
// 	init_board(dim);

// 	/*While the game isn't over*/
// 	while (!done){

// 		// Event is a click of the mouse
// 		while (SDL_PollEvent(&event)) {
// 			switch (event.type) {
// 				// if someone clics on the x, game ends
// 				case SDL_QUIT: {
// 					done = SDL_TRUE;
// 					break;
// 				}
// 				// if someone clicks on the board, it's a play
// 				case SDL_MOUSEBUTTONDOWN:{
// 					int board_x, board_y;
// 					// get place where the mouse clicked
// 					get_board_card(event.button.x, event.button.y, &board_x, &board_y);

// 					printf("click (%d %d) -> (%d %d)\n", event.button.x, event.button.y, board_x, board_y);

// 					// process play
// 					play_response resp = board_play(board_x, board_y);

// 					//renders the card given the type of play
// 					switch (resp.code) {
// 						// first play, only renders the card chosen
// 						case 1:
// 							paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
// 							write_card(resp.play1[0], resp.play1[1], resp.str_play1, 200, 200, 200);
// 							update_board_place(resp.play1[0], resp.play1[1], 0, resp.code);
// 							break;

// 						//game ends
// 						case 3:
// 						  done = 1;

// 						// Correct 2nd play
// 						case 2:
// 							paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
// 							write_card(resp.play1[0], resp.play1[1], resp.str_play1, 0, 0, 0);
// 							update_board_place(resp.play1[0], resp.play1[1], 0, resp.code);
// 							paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
// 							write_card(resp.play2[0], resp.play2[1], resp.str_play2, 0, 0, 0);
// 							update_board_place(resp.play2[0], resp.play2[1], 0, resp.code);
// 							break;
// 						// incorrect 2nd play, renders cards for 2 seconds and then paint white again
// 						case -2:
// 							paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
// 							write_card(resp.play1[0], resp.play1[1], resp.str_play1, 255, 0, 0);
// 							update_board_place(resp.play1[0], resp.play1[1], 0, resp.code);
// 							paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
// 							write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);
// 							update_board_place(resp.play2[0], resp.play2[1], 0, resp.code);
// 							sleep(2);
// 							paint_card(resp.play1[0], resp.play1[1] , 255, 255, 255);
// 							paint_card(resp.play2[0], resp.play2[1] , 255, 255, 255);
// 							break;
// 					}
// 				}
// 			}
// 		}
// 	}
	

// 	close_board_windows();

// }

void* process_players(void* args){
	int* aux_id = args;
	int id = *aux_id;
	int p;
	board_place* board = get_board();
	board_place place;

    write(players_fd[id], &dim, sizeof(dim));

    write(players_fd[id], board, sizeof(board_place)* dim * dim);

    /*Check if player closes*/
    while(read(players_fd[id], &p, sizeof(p)) != 0){

    	play_response resp = board_play(p);

    	//renders the card given the type of play
		switch (resp.code) {
			// first play, only renders the card chosen
			case 1:
				//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
				//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 200, 200, 200);
				update_board_place(resp.play1[0], resp.play1[1], id, resp.code);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				break;

			//game ends
			case 3:
				done = 1;
				update_board_place(resp.play1[0], resp.play1[1], id, 4);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				update_board_place(resp.play2[0], resp.play2[1], id, 4);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				break;
			// Correct 2nd play
			case 2:
				//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
				//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 0, 0, 0);
				update_board_place(resp.play1[0], resp.play1[1], id, 3);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				//paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
				//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 0, 0, 0);
				update_board_place(resp.play2[0], resp.play2[1], id, 3);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				break;
			// incorrect 2nd play, renders cards for 2 seconds and then paint white again
			case -2:
				//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
				//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 255, 0, 0);
				update_board_place(resp.play1[0], resp.play1[1], id, 2);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				//paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
				//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);
				update_board_place(resp.play2[0], resp.play2[1], id, 2);
    			place = get_board_place(p);
    			write(players_fd[id], &place, sizeof(place));
				sleep(2);
				//paint_card(resp.play1[0], resp.play1[1] , 255, 255, 255);
				update_board_place(resp.play1[0], resp.play1[1], id, 0);
				update_board_place(resp.play2[0], resp.play2[1], id, 0);
				break;
			}


    }



}

void* accept_players(){
	pthread_t players_thread[MAX_PLAYERS];

	//Player 1
	players_fd[0] = accept(sock_fd, NULL, NULL);
	if(players_fd[0] == -1){
		printf("Error accepting connection from player 1.\n");
		exit(-1);
	}
    printf("Player 1 connected, waiting for player 2!\n");
    nr_active_players++;

    pthread_create(&(players_thread[0]), NULL, process_players, (void*) 0);


    pthread_join(players_thread[0], NULL);



}