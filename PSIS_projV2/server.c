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
#include <time.h>

#include "board_library.h" 
#include "UI_library.h"

typedef struct player_node{
	int id;
	color color_players;
	int players_fd;
	pthread_t players_thread;
	double timer;
	int card_count;
	pthread_rwlock_t locks_fd;
	struct player_node* next;
}player_node;

#define PORT 3000
#define INACTIVE -2


int sock_fd;
int nr_active_players = 0;
bool done = false;
struct sockaddr_in local_addr;
int dim;
pthread_t get_players;
pthread_t waiter;
pthread_rwlock_t lock_active_players = PTHREAD_RWLOCK_INITIALIZER; // locks for synchronization
player_node* head; 

//void* process_events();
void* process_players(void* args);
void* accept_players();
void* exit_game();
void broadcast_play(play aux_play);
int who_won();
void broadcast_play_except_one(play aux_play, int exception);
void* turn_cards_white();
void init_player_head();
player_node* add_player(int id);
void remove_player(int id);
player_node* get_player_node(int id);
void clear_players();



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

	init_head();
	init_player_head();

	/*Get colors of each player*/
	//get_colors(color_players);

	/*Creates thread to deal with processes*/
	//pthread_create(&SDL_thread, NULL, process_events, NULL);
	init_board(dim);

	
	pthread_create(&get_players, NULL, accept_players, NULL);

	pthread_create(&exit_thread, NULL, exit_game, NULL);

	pthread_create(&waiter, NULL, turn_cards_white, NULL);	

	printf("End get players\n");

	pthread_join(exit_thread, NULL);

	//pthread_join(SDL_thread, NULL);

	printf("fim\n");
	close(sock_fd);
	clear_memory();
	clear_plays();
	//free(head);
	//clear_players();
}

/*Receives every play from each player*/
void* process_players(void* args){
	int* aux_id = args;
	int id = *aux_id;
	int p, winner;
	board_place* board = get_board();
	board_place place;
	play aux_play;
	player_node *aux;

	aux = get_player_node(id);

	printf("dim: %d  player %d\n", dim, id);

	/*Sends dimension of the board to the player*/
    write(aux->players_fd, &dim, sizeof(dim));

    /*Sends the board to the player*/
    write(aux->players_fd, board, sizeof(board_place)* dim * dim);

    /*Check if player closes*/
    while(read(aux->players_fd, &p, sizeof(p)) != 0){

    	//printf("place %d\n", p;
    	pthread_rwlock_rdlock(&lock_active_players);
    	if(nr_active_players > 1){

	    	play_response resp = board_play(p, id);

	    	//printf("resp code %d\n", resp.code);

	    	//renders the card given the type of play
			switch (resp.code) {
				// first play, only renders the card chosen
				case 1:
					update_board_place(resp.play1[0], resp.play1[1], id, 1);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			aux->timer = (double) 5;
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
	    			aux = get_player_node(winner);
	    			write(aux->players_fd, &aux_play, sizeof(play));

	    			sleep(10);
	    			clear_memory();
	    			init_board(dim);
					break;
				// Correct 2nd play
				case 2:
					printf("play correct: %d %d \n",resp.play1[0], resp.play1[1] );
					update_board_place(resp.play1[0], resp.play1[1], id, 3);
	    			place = get_board_place(p);
	    			aux_play.x = resp.play1[0];
	    			aux_play.y = resp.play1[1];
	    			aux_play.place = place;
	    			//write(players_fd[id], &aux_play, sizeof(play));
					update_board_place(resp.play2[0], resp.play2[1], id, 3);
					p = resp.play2[0] + resp.play2[1] * dim;
	    			place = get_board_place(p);
	    			aux_play.x = resp.play2[0];
	    			aux_play.y = resp.play2[1];
	    			aux_play.place = place;
	    			aux->card_count++;
	    			broadcast_play(aux_play);
					break;
				// incorrect 2nd play, renders cards for 2 seconds and then paint white again
				case -2:
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


    	pthread_rwlock_unlock(&lock_active_players);

    }

    /*Closes the socket, removes the play and player from the lists and decrements the nr of active players*/
    close(aux->players_fd);
    aux->players_fd = INACTIVE;
    remove_play(id);
    remove_player(id);
    pthread_rwlock_wrlock(&lock_active_players);
    nr_active_players--;
    pthread_rwlock_unlock(&lock_active_players);
    pthread_detach(pthread_self());

}

/*Receives connection from other players*/
void* accept_players(){
	int player = 0, aux_player;
	player_node *auxI, *aux, *aux2;

	/*Create socket*/
	sock_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_fd == -1){
		perror("socket: ");
	    exit(-1);
	}

	/*Sets local address*/
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

	printf("Waiting for players...\n");

	while(1){

		//Find available player id
		for(int i = 0; 1; i++){
			auxI = get_player_node(i);
			if(auxI == NULL){
				player = i;
				break;
			}
			
		}

		printf("player %d to join\n", player);
		aux = add_player(player);

		aux->players_fd = accept(sock_fd, NULL, NULL);
		if(aux->players_fd == -1){
			printf("Error accepting connection from player 1.\n");
			exit(-1);
		}
		aux2 = aux;
		aux_player = player;

		add_play(aux_player);

	    pthread_rwlock_wrlock(&lock_active_players);
	    nr_active_players++;
	    pthread_rwlock_unlock(&lock_active_players);

	    printf("player %d joined\n", aux_player);

	    pthread_create(&(aux2->players_thread), NULL, process_players, (void*) &aux_player);

	}
    //pthread_join(players_thread[0], NULL);	


}


/*Terminates active threads*/
void* exit_game(){

	char c;
	player_node *aux;

	while(1){

		c = getchar();

		if(c == 'q'){

			for(aux = head->next; aux != NULL; aux = aux->next){
				pthread_cancel(aux->players_thread);
			}

			pthread_cancel(get_players);
			//clear_memory();
			//exit(1);
			break;
		}
	}
}

/*Sends play to every plare*/
void broadcast_play(play aux_play){
	player_node *aux;

	for(aux = head->next; aux != NULL; aux = aux->next){
		//pthread_rwlock_rdlock(&(aux->locks_fd));
		write(aux->players_fd, &aux_play, sizeof(play));
		
		//pthread_rwlock_unlock(&(aux->locks_fd));
	}
}

/*Verifies who won the game*/
int who_won(){
	int max = 0, winner;
	player_node *aux;

	for(aux = head->next; aux != NULL; aux = aux->next){
		if(max < aux->card_count)
		{
			max = aux->card_count;
			winner = aux->id;
		}

		//Zero card count for next game
		aux->card_count = 0;
	}

	return winner;
}

/*Sends play to every player except one (case where we have to send a different play to the winner)*/
void broadcast_play_except_one(play aux_play, int exception){
	player_node *aux;

	for(aux = head->next; aux != NULL; aux = aux->next){
		pthread_rwlock_rdlock(&(aux->locks_fd));
		if(aux->id != exception){
			write(aux->players_fd, &aux_play, sizeof(play));
		}
		pthread_rwlock_unlock(&(aux->locks_fd));

	}
}

/*Thread that checks if 5 seconds passed after the first play of the player. If it happens, the card is turned white. */
void* turn_cards_white(){

	double now;
	double aux_now;
	play aux_play;
	player_node *aux;

	now = (double) clock()/CLOCKS_PER_SEC;


	while(!done){

		aux_now = (double) clock()/CLOCKS_PER_SEC - now;
		now = (double) clock()/CLOCKS_PER_SEC;

		//printf("time  %lf\n", aux_now);

		for(aux = head->next; aux != NULL; aux = aux->next){
			if(aux->timer > 0){
				
				aux->timer -= aux_now;


				if(aux->timer <= 0){
					aux_play = last_played(aux->id);

					broadcast_play(aux_play);
				}
			}
		}
	}

	pthread_detach(pthread_self());

}

/*Initializes list of players*/
void init_player_head(){
	head = malloc(sizeof(player_node));
	head->id = -1;
	head->next = NULL;
}

/*Adds a player to the list of players*/
player_node* add_player(int id){

	player_node* new;

	new = malloc(sizeof(player_node));

	new->id = id;
	pthread_rwlock_init(&(new->locks_fd), NULL);
	new->color_players = get_single_color(id);
	new->timer = 0;
	new->card_count = 0;
	new->players_fd= INACTIVE;
	new->next = head->next;
	head->next = new;

	return new;
}

/*Removes player from the list of players*/
void remove_player(int id){

	player_node *aux, *removed;

	for(aux = head; aux!= NULL; aux = aux->next){
		if(aux->next->id == id){
			removed = aux->next;
			aux->next = aux->next->next;
			free(removed);
			break;
		}
	}
}

/*Returns player given its id*/
player_node* get_player_node(int id){

	player_node *aux;

	for(aux = head; aux != NULL; aux = aux->next){
		if(aux->id == id){
			return aux;
		}
	}

	return NULL;
}

/*Frees list*/
void clear_players(){
	player_node * aux, *aux2;

	for(aux = head; aux != NULL;){
		aux2 = aux;
		aux = aux->next;
		free(aux2);
	}
}