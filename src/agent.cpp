/*********************************************
 *  agent.cpp
 *  UNSW Session 1, 2015
 *  Gabriel Low & Yilser Kebabran
*/

#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "pipe.h"

#define map_size 80
#define world_size map_size * 2 - 1

int   pipe_fd;
FILE* in_stream;
FILE* out_stream;

char view[5][5];

class Map {
	int posX, posY;
	int seenXMin, seenXMax, seenYMin, seenYMax;
public:
	char world[world_size][world_size];
	
	Map();
	void updateMap(int x, int y, char view[5][5]);
	void print();
};

Map::Map() {
	posX = 0;
	posY = 0;
	
	seenXMin = 0;
	seenXMax = 0;
	seenYMin = 0;
	seenYMax = 0;
	
	for (int i = 0; i < world_size; ++i) {
		for (int j = 0; j < world_size; ++j) {
			world[i][j] = '?';
		}
	}
}

void Map::updateMap(int x, int y, char view[5][5]) {
	// Update seen area
	seenXMin = x - 2 < seenXMin ? x - 2 : seenXMin;
	seenXMax = x + 2 > seenXMax ? x + 2 : seenXMax;
	seenYMin = y - 2 < seenYMin ? y - 2 : seenYMin;
	seenYMax = y + 2 > seenYMax ? y + 2 : seenYMax;
	
	// Offset for array
	x += 78;
	y += 78; 
	
	// Update map
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			world[x + i][y + j] = view[i][j];
		}
	}
	
	// world ignores player
	// TODO: needs account for boat
	world[x + 2][y + 2] = ' ';
}

void Map::print() {
	int arrayXMin = seenXMin + 80;
	int arrayXMax = seenXMax + 80;
	int arrayYMin = seenYMin + 80;
	int arrayYMax = seenYMax + 80;
	
	for (int i = arrayYMin; i <= arrayYMax; ++i) {
		for (int j = arrayXMin; j <= arrayXMax; ++j) {
			putchar(world[i][j]);
		}
		printf("\n");
	}
}

char world[world_size][world_size];
char lastMove;
int posX;
int posY;
int heading;

void transpose(char (&tran)[5][5]) {
	int i, j;
	char swap;
	for (i = 1; i < 5; ++i) {
		for (j = 0; j < i; ++j) {
			swap = tran[i][j];
			tran[i][j] = tran[j][i];
			tran[j][i] = swap;
		}
	}
}

void reverseRows(char (&rev)[5][5]) {
	int i, start, end;
	char swap;
	for (i = 0; i < 5; ++i) {
		start = 0;
		end = 4;
		while (start < end) {
			swap = rev[i][start];
			rev[i][start] = rev[i][end];
			rev[i][end] = swap;
			++start;
			--end;
		}
	}
}

void reverseCols(char (&rev)[5][5]) {
	int i, start, end;
	char swap;
	for (i = 0; i < 5; ++i) {
		start = 0;
		end = 4;
		while (start < end) {
			swap = rev[start][i];
			rev[start][i] = rev[end][i];
			rev[end][i] = swap;
			++start;
			--end;
		}
	}
}

void rotateCW(char (&rot)[5][5]) {
	transpose(rot);
	reverseRows(rot);
}

void rotateCCW(char (&rot)[5][5]) {
	transpose(rot);
	reverseCols(rot);
}

void rotate180(char (&rot)[5][5]) {
	reverseCols(rot);
	reverseRows(rot);
}

void print_view(char (&dis)[5][5]) {
	int i, j;
	
	printf("\n+-----+\n");
	for (i = 0; i < 5; i++) {
		putchar('|');
		for (j = 0; j < 5; j++) {
			putchar(dis[i][j]);
		}
		printf("|\n");
	}
	printf("+-----+\n");
}

void print_map() {
	int i, j;
	
	for (i = 0; i < world_size; i++) {
		putchar('|');
		for (j = 0; j < world_size; j++) {
			if ((i == posX) && (j == posY)) {
				putchar('X');
			} else {
				putchar(world[i][j]);
			}
		}
		printf("|\n");
	}
}

char get_action(char view[5][5]) {
	char copy[5][5];
	memcpy(copy, view, sizeof (char) * 5 * 5);
	switch (heading) {
		case 1:
			rotateCW(copy);
			copy[2][2] = '>';
			break;
		case 2:
			rotate180(copy);
			copy[2][2] = 'v';
			break;
		case 3:
			rotateCCW(copy);
			copy[2][2] = '<';
			break;
		default:
			copy[2][2] = '^';
			break;
	}
	
	print_view(copy);
	print_map();
	
	if (view[1][2] == '~' || view[1][2] == '*'|| view[1][2] == 'T') {
		lastMove = 'r';
		heading = (heading + 1)%4;
	} else {
		lastMove = 'f';
	}
	
	usleep(500000);
	
	return lastMove;
}

int main(int argc, char *argv[]) {
	char action;
	int sd;
	int ch;
	int i, j;
	lastMove = '?';
	posX = map_size;
	posY = map_size;
	heading = 0;
	
	if (argc < 3) {
		printf("Usage: %s -p port\n", argv[0] );
		exit(1);
	}
	
	// open socket to Game Engine
	sd = tcpopen(atoi(argv[2]));
	
	pipe_fd    = sd;
	in_stream  = fdopen(sd,"r");
	out_stream = fdopen(sd,"w");
	
	for (i = 0; i < world_size; ++i) {
		for (j = 0; j < world_size; ++j) {
			world[i][j] = '?';
		}
	}
	
	while (1) {
		// scan 5-by-5 wintow around current location
		for (i = 0; i < 5; ++i) {
			for (j = 0; j < 5; ++j) {
				if ((i != 2) || (j != 2)) {
					ch = getc( in_stream );
					if (ch == -1) {
						exit(1);
					}
					view[i][j] = ch;
				}
			}
		}
		
		action = get_action(view);
		putc(action, out_stream);
		fflush(out_stream);
	}
	
	return 0;
}
