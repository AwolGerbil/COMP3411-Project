/*********************************************
 *  agent.cpp
 *  UNSW Session 1, 2015
 *  Gabriel Low & Yilser Kabaran
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

class World {
	int posX, posY; // cartesian coordinates
	int direction; // 0 = north, 1 = east, 2 = south, 3 = west
	int seenXMin, seenXMax, seenYMin, seenYMax; // rectangular bounds of visible area in cartesian coordinates
public:
	char map[world_size][world_size];
	
	World();
	void updateMap(char (&view)[5][5]);
	void move(char command);
	void print();
	
	char getFront();
        void clearFront();
	
	int getPositionX() { return posX; }
	int getPositionY() { return posY; }
	int getVisibleWidth() { return seenXMax - seenXMin; }
	int getVisibleHeight() { return seenYMax - seenYMin; }
};

World::World() {
	posX = 0; // starts at (0, 0)
	posY = 0;
	direction = 2; // starts facing south
	
	seenXMin = 0;
	seenXMax = 0;
	seenYMin = 0;
	seenYMax = 0;
	
	for (int i = 0; i < world_size; ++i) {
		for (int j = 0; j < world_size; ++j) {
			map[i][j] = '?';
		}
	}
}

void World::updateMap(char (&view)[5][5]) {
	// Update seen area
	seenXMin = posX - 2 < seenXMin ? posX - 2 : seenXMin;
	seenXMax = posX + 2 > seenXMax ? posX + 2 : seenXMax;
	seenYMin = posY - 2 < seenYMin ? posY - 2 : seenYMin;
	seenYMax = posY + 2 > seenYMax ? posY + 2 : seenYMax;
	
	// Offset for array
	int x = posX + 78;
	int y = posY + 82;
	
	// Rotate view to correct orientation
	if (direction == 1) {
		rotateCW(view);
	} else if (direction == 2) {
		rotate180(view);
	} else if (direction == 3) {
		rotateCCW(view);
	}
	
	// Update map
	for (int i = 0; i < 5; ++i) {
		for (int j = 0; j < 5; ++j) {
			map[x + j][y - i] = view[i][j];
		}
	}
	
	// world map ignores player
	// TODO: needs account for boat
	map[posX + 80][posY + 80] = ' ';
}

void World::move(char command) {
	if (command == 'F' || command == 'f') { // Step forward
		if (direction == 0) {
			++posY;
		} else if (direction == 1) {
			++posX;
		} else if (direction == 2) {
			--posY;
		} else {
			--posX;
		}
	} else if (command == 'L' || command == 'l') { // Turn left
		direction = (direction + 3) % 4;
	} else if (command == 'R' || command == 'r') { // Turn right
		direction = (direction + 1) % 4;
	} else if (command == 'C' || command == 'c') { // Chop
	        if(getFront() == 'T'){
                    clearFront();
                }
	} else if (command == 'B' || command == 'b') { // BOOOOOOOOOOM!
	        if(getFront() == 'T' || getFront() == '*'){
                    clearFront();
                }
	} else {
		printf("Y U DO DIS?");
	}
}

void World::print() {
	int arrayXMin = seenXMin + 80;
	int arrayXMax = seenXMax + 80;
	int arrayYMin = seenYMin + 80;
	int arrayYMax = seenYMax + 80;
	for (int i = arrayXMin - 1; i <= arrayXMax + 1; ++i) { putchar('?'); }
	printf("\n");
	for (int j = arrayYMax; j >= arrayYMin; --j) {
		putchar('?');
		for (int i = arrayXMin; i <= arrayXMax; ++i) {
			if (i == posX + 80 && j == posY + 80) {
				if (direction == 0) { putchar('^'); }
				else if (direction == 1) { putchar('>'); }
				else if (direction == 2) { putchar('v'); }
				else { putchar('<'); }
			} else {
				putchar(map[i][j]);
			}
		}
		printf("?\n");
	}
	for (int i = arrayXMin - 1; i <= arrayXMax + 1; ++i) { putchar('?'); }
	printf("\n");
}

char World::getFront() {
	if (direction == 0) {
		return map[posX + 80][posY + 81];
	} else if (direction == 1) {
		return map[posX + 81][posY + 80];
	} else if (direction == 2) {
		return map[posX + 80][posY + 79];
	} else {
		return map[posX + 79][posY + 80];
	}
}

void World::clearFront() {
	if (direction == 0) {
		map[posX + 80][posY + 81] = ' ';
	} else if (direction == 1) {
		map[posX + 81][posY + 80] = ' ';
	} else if (direction == 2) {
		map[posX + 80][posY + 79] = ' ';
	} else {
		map[posX + 79][posY + 80] = ' ';
	}
}

class State{
private:
	int x;
	int y;
	int cost;
	int est;
	int dyna_count;
	bool axe;
	bool gold;
	bool boat;
public:
	State(int x_, int y_, int cost_, int est_, int dyna_count, bool axe_, bool gold_, bool boat_);
	int getTotalCost(){ return cost+est; };
	int getX(){ return x; };
	int getY(){ return y; };
	int getCost(){ return cost; };
	int getEst(){ return est; };
	int getDynaCount(){ return dyna_count; };
	bool haveAxe(){ return axe; };
	bool haveGold(){ return gold; };
	bool haveBoat(){ return boat; };
};

State::State(int x_, int y_, int cost_, int est_, int dyna_count_, bool axe_, bool gold_, bool boat_){
	x = x_;
	y = y_;
	cost = cost_;
	est = est_;
	dyna_count = dyna_count_;
	axe = axe_;
	gold = gold_;
	boat = boat_;
}

char getAction(World &world) {
	world.print();
	printf("%d %d", world.getPositionX(), world.getPositionY());
	
	char front = world.getFront();
	char move;
	if (front == 'T' || front == '*' || front == '~') {
		move = 'r';
	} else {
		move = 'f';
	}
	getchar();
	world.move(move);
	return move;
}

int main(int argc, char *argv[]) {
	char action;
	int sd;
	int ch;
	int i, j;
	World world = World();
	
	if (argc < 3) {
		printf("Usage: %s -p port\n", argv[0] );
		exit(1);
	}
	
	// open socket to Game Engine
	sd = tcpopen(atoi(argv[2]));
	
	pipe_fd    = sd;
	in_stream  = fdopen(sd,"r");
	out_stream = fdopen(sd,"w");
	
	char view[5][5];
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
				} else {
					view[i][j] = '^';
				}
			}
		}
		world.updateMap(view);
		
		action = getAction(world);
		putc(action, out_stream);
		fflush(out_stream);
	}
	return 0;
}
