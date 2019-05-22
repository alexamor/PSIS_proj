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
pthread_t get_players;
pthread_t players_thread[MAX_PLAYERS];
int card_count[MAX_PLAYERS] = {0};

//void* process_events();
void* process_players(void* args);
void* accept_players();
void* exit_game();
void broadcast_play(play aux_play);
int who_won();
void broadcast_play_except_one(play aux_play, int exception);


int main(int argc, char* argv[]){
	// server variables

	pthread_t exit_thread;
	
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
	init_board(dim);

	
	pthread_create(&get_players, NULL, accept_players, NULL);

	pthread_create(&exit_thread, NULL, exit_game, NULL);	

	printf("End get players\n");

	pthread_join(exit_thread, NULL);

	//pthread_join(SDL_thread, NULL);

	printf("fim\n");
	close(sock_fd);
	clear_memory();
}


void* process_players(void* args){
	int* aux_id = args;
	int id = *aux_id;
	int p, winner;
	board_place* board = get_board();
	board_place place;
	play aux_play;

	printf("dim: %d  player %d\n", dim, id);

    write(players_fd[id], &dim, sizeof(dim));

    write(players_fd[id], board, sizeof(board_place)* dim * dim);

    /*Check if player closes*/
    while(read(players_fd[id], &p, sizeof(p)) != 0){

    	//printf("place %d\n", p);

    	if(nr_active_players > 1){

	    	play_response resp = board_play(p, id);

	    	//printf("resp code %d\n", resp.code);

	    	//renders the card given the type of play
			switch (resp.code) {
				// first play, only renders the card chosen
				case 1:
					//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
					//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 200, 200, 200);
					update_board_place(resp.play1[0], resp.play1[1], id, 1);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			broadcast_play(aux_play);
					break;
				//game ends
				case 3:
					done = 1;
					update_board_place(resp.play1[0], resp.play1[1], id, 4);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			//write(players_fd[id], &aux_play, sizeof(play));
					update_board_place(resp.play2[0], resp.play2[1], id, 4);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play2[0];
	    			aux_play.y = resp.play2[1];
	    			aux_play.place = place;

	    			//players receive generic end game message
	    			winner = who_won();

	    			broadcast_play_except_one(aux_play, winner);

	    			//winner receives winning message
	    			aux_play.place.state = 5;
	    			write(players_fd[winner], &aux_play, sizeof(play));

	    			sleep(10);
	    			clear_memory();
	    			init_board(dim);
					break;
				// Correct 2nd play
				case 2:
					//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
					//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 0, 0, 0);
					update_board_place(resp.play1[0], resp.play1[1], id, 3);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			//write(players_fd[id], &aux_play, sizeof(play));
					//paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
					//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 0, 0, 0);
					update_board_place(resp.play2[0], resp.play2[1], id, 3);
					p = resp.play2[0] + resp.play2[1] * dim;
	    			place = get_board_place(p);
	    			aux_play.x = resp.play2[0];
	    			aux_play.y = resp.play2[1];
	    			aux_play.place = place;
	    			card_count[id]++;
	    			broadcast_play(aux_play);
					break;
				// incorrect 2nd play, renders cards for 2 seconds and then paint white again
				case -2:
					//paint_card(resp.play1[0], resp.play1[1] , color_players[0].r, color_players[0].g, color_players[0].b);
					//write_card(resp.play1[0], resp.play1[1], resp.str_play1, 255, 0, 0);
					update_board_place(resp.play1[0], resp.play1[1], id, 2);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			//write(players_fd[id], &aux_play, sizeof(play));
					//paint_card(resp.play2[0], resp.play2[1] , color_players[0].r, color_players[0].g, color_players[0].b);
					//write_card(resp.play2[0], resp.play2[1], resp.str_play2, 255, 0, 0);
					update_board_place(resp.play2[0], resp.play2[1], id, 2);
					p = resp.play2[0] + resp.play2[1] * dim;
	    			place = get_board_place(p);
	    			aux_play.x = resp.play2[0];
	    			aux_play.y = resp.play2[1];
	    			aux_play.place = place;
	    			broadcast_play(aux_play);
					sleep(2);
					//paint_card(resp.play1[0], resp.play1[1] , 255, 255, 255);
					update_board_place(resp.play1[0], resp.play1[1], id, 0);
					update_board_place(resp.play2[0], resp.play2[1], id, 0);
					break;
			}
    	}



    }

    close(players_fd[id]);
    players_fd[id] = INACTIVE;
    nr_active_players--;
    pthread_detach(pthread_self());

}

void* accept_players(){
	
	int i = 0;
	int player = 0, aux_player;

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

	while(1){

		if(nr_active_players < (MAX_PLAYERS - 1)){

			for(int i = 0; i < MAX_PLAYERS; i++){
				if(players_fd[i] == INACTIVE){
					player = i;
					break;
				}
			}

			printf("player %d to join\n", player);
			//Player 1	
			players_fd[player] = accept(sock_fd, NULL, NULL);
			if(players_fd[player] == -1){
				printf("Error accepting connection from player 1.\n");
				exit(-1);
			}
			aux_player = player;

		    nr_active_players++;
		    printf("player %d joined\n", aux_player);

		    pthread_create(&players_thread[aux_player], NULL, process_players, (void*) &aux_player);
		}

	}
    //pthread_join(players_thread[0], NULL);	


}

void* exit_game(){

	char c;

	while(1){

		c = getchar();

		if(c == 'q'){

			for(int i = 0; i < MAX_PLAYERS; i++){
				if(players_fd[i] != INACTIVE)
					pthread_cancel(players_thread[i]);
			}

			pthread_cancel(get_players);
			//clear_memory();
			//exit(1);
			break;
		}
	}
}

void broadcast_play(play aux_play){
	int i = 0;

	for(i = 0; i < MAX_PLAYERS; i++){
		if(players_fd[i] != INACTIVE)
			write(players_fd[i], &aux_play, sizeof(play));
	}
}

int who_won(){
	int max = 0, winner;

	for(int i = 0; i < MAX_PLAYERS; i++){
		if(max < card_count[i])
		{
			max = card_count[i];
			winner = i;
		}

		//Zero card count for next game
		card_count[i] = 0;
	}

	return winner;
}

void broadcast_play_except_one(play aux_play, int exception){
	int i = 0;

	for(i = 0; i < MAX_PLAYERS; i++){
		if((players_fd[i] != INACTIVE) && (i != exception))
			write(players_fd[i], &aux_play, sizeof(play));
	}
}