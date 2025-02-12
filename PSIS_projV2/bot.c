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
	pthread_t exit_thread;
	
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

			printf("Card: %d %d   State: %d\n ", read_play.x, read_play.y, read_play.place.state );

			//renders the card given the type of play
			switch (read_play.place.state) {
				case 5:
				case 4:
					sleep(10);
					for(i = 0; i < board_size; i++){
						for(j = 0; j < board_size; j++){
							paint_card(i, j , 255, 255, 255);
						}
					}
					break;	
			}
		}
	}

	pthread_join(SDL_thread, NULL);
	pthread_join(exit_thread, NULL);

  	close(sock_fd);

  	free(board);

  	return 0;

}


/*Generates a random play and sends it to the server*/
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

/*Exits the game if someone sends the letter 'q' via the terminal*/
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

