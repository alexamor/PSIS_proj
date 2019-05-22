#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <inttypes.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "board_library.h" 
#include "UI_library.h"

#define PORT 3000

bool done = false;
int board_size;
board_place * board;
pthread_t SDL_thread;
int sock_fd;

void* checkForPlays();
void* exit_game();

int main(int argc, char * argv[]){
	struct sockaddr_in server_addr;
	play read_play;
	color player_color;
	pthread_t exit_thread;
	
	int red_cards = 0;
	int toTurnCards[MAX_PLAYERS][2][2];
	char prev_card[MAX_PLAYERS][3];
	int i, j;

	

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


    read(sock_fd, &board_size, sizeof(board_size));
    printf("board_size %d\n", board_size);
    

    //create_board_window(300, 300, board_size);

    board = (board_place *) malloc(sizeof(board_place)* board_size * board_size);

    printf("Board allocated\n");

    read(sock_fd, board, sizeof(board_place) * board_size * board_size);

    printf("Board read\n");

    //draw_board();

    //printf("Board draw\n");
	
	//threads here
	pthread_create(&SDL_thread, NULL, checkForPlays, NULL);
	pthread_create(&exit_thread, NULL, exit_game, NULL);

	printf("thread checkForPlays created\n");


	while(done == false && read(sock_fd, &read_play, sizeof(read_play)) != 0){
		if(1){

			printf("Play read\n");

			player_color = get_single_color(read_play.place.player);

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
					toTurnCards[read_play.place.player][0][0] = read_play.x;
					toTurnCards[read_play.place.player][0][1] = read_play.y;
					strcpy(prev_card[read_play.place.player], read_play.place.v);
					break;
				//wrong card up - letters red
				case 2:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, read_play.place.v, 255, 0, 0);
					write_card(toTurnCards[read_play.place.player][0][0], toTurnCards[read_play.place.player][0][1], prev_card[read_play.place.player], 255, 0, 0);
					red_cards++;
					toTurnCards[read_play.place.player][1][0] = read_play.x;
					toTurnCards[read_play.place.player][1][1] = read_play.y;
					break;
				//locked card - letters black
				case 3:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, prev_card[read_play.place.player], 0, 0, 0);
					write_card(toTurnCards[read_play.place.player][0][0], toTurnCards[read_play.place.player][0][1], prev_card[read_play.place.player], 0, 0, 0);
					break;
				//win - board completed
				case 4:
					paint_card(read_play.x, read_play.y, player_color.r, player_color.g, player_color.b);
					write_card(read_play.x, read_play.y, prev_card[read_play.place.player], 0, 0, 0);
					write_card(toTurnCards[read_play.place.player][0][0], toTurnCards[read_play.place.player][0][1], prev_card[read_play.place.player], 0, 0, 0);
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
				paint_card(toTurnCards[read_play.place.player][0][0], toTurnCards[read_play.place.player][0][1], 255, 255, 255);
				paint_card(toTurnCards[read_play.place.player][1][0], toTurnCards[read_play.place.player][1][1], 255, 255, 255);
				red_cards = 0;
			}


		}
	}

	pthread_join(SDL_thread, NULL);
	pthread_join(exit_thread, NULL);

  	close(sock_fd);

  	free(board);

  	return 0;

}


void* checkForPlays(){

	int p;

	srand(time(NULL));

	/*While the game isn't over*/
	while (!done){

		sleep(2);

		p = abs(rand()%(board_size*board_size));

		printf("p %d\n", p);

		//Send play to server
		write(sock_fd, &p, sizeof(p));			
	}

	printf("fim\n");
	//close_board_windows();
}

void* exit_game(){

	char c;

	while(1){

		c = getchar();

		if(c == 'q'){

			pthread_cancel(SDL_thread);

			//break;

			close(sock_fd);

  			free(board);

  			exit(1);

		}
	}
}

