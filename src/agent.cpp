/*********************************************
 *  agent.cpp
 *  UNSW Session 1, 2015
 *  Gabriel Low & Yilser Kabaran
*/

#include <string.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include <algorithm>
#include <queue>
#include <vector>

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

class Inventory{
	bool axe, gold;
	int kaboomCount;
public:
	Inventory();
	bool getAxe(){return axe;}
	bool getGold(){return gold;}
	int getKaboomCount(){return kaboomCount;}
	void setGold(bool gold){this->gold = gold;}
	void setAxe(bool axe){this->axe = axe;}
	void setKaboomCount(bool count){this->kaboomCount = count;}
};

Inventory::Inventory(){
	axe = false;
	gold = false;
	kaboomCount = 0;
}

class World {
	Inventory inventory;
	int posX, posY; // cartesian coordinates
	int direction; // 0 = north, 1 = east, 2 = south, 3 = west
	int seenXMin, seenXMax, seenYMin, seenYMax; // rectangular bounds of visible area in cartesian coordinates
	int aStarDestX, aStarDestY; // last aStar destination
	std::vector<char> aStarCache; // for caching the shortest path to a given coordinate
public:
	char map[world_size][world_size];
	static const int forwardX[4];
	static const int forwardY[4];
	
	World();
	void updateMap(char (&view)[5][5]);
	void move(char command);
	char aStar(int destX, int destY);
	char explore();
	char findTile(char target);
	void print() const;
	
	char getFront() const;
	void clearFront();
	
	int getPositionX() const { return posX; }
	int getPositionY() const { return posY; }
	int getVisibleWidth() const { return seenXMax - seenXMin; }
	int getVisibleHeight() const { return seenYMax - seenYMin; }
	
	char getMap(int x, int y) const { return map[x + 80][y + 80]; }

	bool getGold() { return inventory.getGold(); }
	bool getAxe() { return inventory.getAxe(); }
};

const int World::forwardX[4] = {0, 1, 0, -1};
const int World::forwardY[4] = {1, 0, -1, 0};

World::World() {
	inventory = Inventory();
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
	// view (matrix coordinates) top left to bottom right = (0, 0), (0, 1) ... (4, 3), (4, 4)
	// map (cartesian coordinates) top left to bottom right = (-2, 2), (-1, 2) ... (1, -2), (2, -2)
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
		switch(getFront()){
			case 'a':{
				inventory.setAxe(true);
				break;
				}
			case 'd':{
				int count = inventory.getKaboomCount();
				count++;
				inventory.setKaboomCount(count);
				break;
				}
			case 'g':{
				inventory.setGold(true);
				break;
				}
		}
		posX = posX + forwardX[direction];
		posY = posY + forwardY[direction];
	} else if (command == 'L' || command == 'l') { // Turn left
		direction = (direction + 3) % 4;
	} else if (command == 'R' || command == 'r') { // Turn right
		direction = (direction + 1) % 4;
	} else if (command == 'C' || command == 'c') { // Chop
		if (getFront() == 'T') {
			clearFront();
		}
	} else if (command == 'B' || command == 'b') { // BOOOOOOOOOOM!
		if (getFront() == 'T' || getFront() == '*') {
			clearFront();
		}
	} else {
		printf("Y U DO DIS?");
	}
}

class aStarNode {
public:
	int posX, posY, direction, destX, destY;
	std::vector<char> path;
	
	aStarNode(const int posX, const int posY, const int direction, const int destX, const int destY, const std::vector<char> &path = std::vector<char>()) {
		this->posX = posX;
		this->posY = posY;
		this->direction = direction;
		this->destX = destX;
		this->destY = destY;
		this->path = path;
	}
	
	aStarNode(const aStarNode &old, const World &world, const char move) {
		posX = old.posX;
		posY = old.posY;
		direction = old.direction;
		if (move == 'F' || move == 'f') {
			char front = world.getMap(posX + world.forwardX[direction], posY + world.forwardY[direction]);
			if (front == ' ' || front == 'g' || front == 'a' || front == 'd' || front == 'B'
				|| (front == '~' && (world.getMap(posX, posY) == 'B' || world.getMap(posX, posY) == '~'))) {
				posX += world.forwardX[direction];
				posY += world.forwardY[direction];
			}
		} else if (move == 'L' || move == 'l') {
			direction = (direction + 3) % 4;
		} else if (move == 'R' || move == 'r') {
			direction = (direction + 1) % 4;
		}
		
		destX = old.destX;
		destY = old.destY;
		path = old.path;
		path.push_back(move);
	}
	
	int estimate() const {
		int dX = destX - posX;
		int dY = destY - posY;
		if (posX != destX && posY != destY) {
			return (dX < 0 ? -dX : dX) + (dY < 0 ? -dY : dY) + 1;
		} else if (posX == destX) {
			return (dX < 0 ? -dX : dX) + (dY < 0 ? -dY : dY) + ((dY == 0) || (dY > 0 && direction == 0) || (dY < 0 && direction == 2) ? 0 : 1);
		} else {
			return (dX < 0 ? -dX : dX) + (dY < 0 ? -dY : dY) + ((dX > 0 && direction == 1) || (dX < 0 && direction == 3) ? 0 : 1);
		}
	}
	
	bool operator==(const aStarNode &other) const {
		return (posX == other.posX && posY == other.posY && direction == other.direction);
	}
	
	bool operator<(const aStarNode &other) const {
		return path.size() + estimate() < other.path.size() + other.estimate();
	}
	
	bool operator>(const aStarNode &other) const {
		return path.size() + estimate() > other.path.size() + other.estimate();
	}
};

// Returns the optimal move given a destination
// Does not consider picking up tools
// If unpathable, returns 0
char World::aStar(int destX, int destY) {
	// Use cached path if possible
	if (destX == aStarDestX && destY == aStarDestY) {
		char move = aStarCache.back();
		aStarCache.pop_back();
		return move;
	}
	
	if (getMap(destX, destY) == 'T' || getMap(destX, destY) == '*') {
		return 0;
	}
	
	std::vector<aStarNode> closed;
	std::priority_queue<aStarNode, std::vector<aStarNode>, std::greater<aStarNode> > open;
	aStarNode current(posX, posY, direction, destX, destY);
	open.push(current);
	
	while (!open.empty()) {
		current = open.top();
		
		// YAAAY!
		if (current.estimate() == 0) {
			// Update cache
			aStarDestX = destX;
			aStarDestY = destY;
			char move = current.path.front();
			std::reverse(current.path.begin(), current.path.end());
			current.path.pop_back();
			aStarCache = current.path;
			return move;
		}
		
		// Pop off open set and add to closed set
		open.pop();
		closed.push_back(current);
		
		// Add neighbours (move forward, turn left/right)
		char testMoves[3] = {'f', 'l', 'r'};
		for (int i = 0; i < 3; ++i) {
			aStarNode nextNode = aStarNode(current, *this, testMoves[i]);
			// Check if in closed set
			bool found = false;
			for (std::vector<aStarNode>::iterator iter = closed.begin(); iter != closed.end(); ++iter) {
				if (nextNode == *iter) {
					found = true;
					break;
				}
			}
			
			if (!found) open.push(nextNode);
		}
	}
	
	return 0;
};

class ExpNode{
public:
	int posX, posY, moves;
	World* world;

	ExpNode(const int posX, const int posY, World* world, const int moves){
		this->posX = posX;
		this->posY = posY;
		this->world = world;
		this->moves = moves;
	}
	
	int surrounds() const {
		int count = 0;
		for(int i = posX-2; i <= posX+2; i++){
			for(int j = posY-2; j <= posY+2; j++){
				if (world->getMap(i,j) == '?'){
					count++;
				}
			}
		}
		return count;
	}
	
	bool operator==(const ExpNode &other) const {
		return (posX == other.posX && posY == other.posY);
	}
	
	bool operator<(const ExpNode &other) const {
		return moves < other.moves;
	}
	
	bool operator>(const ExpNode &other) const {
		return moves > other.moves;
	}

};

char World::explore(){
	int destX ,destY = 0;
	scanf("%d %d", &destX, &destY);
	char move =  aStar(destX,destY);
	while(move == 0){
		printf("nop\n");
		scanf("%d %d", &destX, &destY);
		move =  aStar(destX,destY);
	}
	return move;
	std::vector<ExpNode> closed;
	std::priority_queue<ExpNode> open;

	ExpNode current(posX, posY,this,0);
	open.push(current);
	//directions of movement arraged so once added to the queue the ones with least turns required
	//will be sorted closer to front if tied with others
	while (!open.empty()){
		current = open.top();
		if (current.surrounds() > 0){
			char move = aStar(current.posX, current.posY);
			printf("\ncan I get there? %d %d\n",current.posX,current.posY);
			if (move != 0){
				return move;
			}
			printf("no");
		}

		open.pop();
		closed.push_back(current);
		for(int i = 0; i < 4; i++){
			int x = current.posX + forwardX[i];
			int y = current.posY + forwardY[i];
			switch (getMap(x,y)){
				case 'T':
				case '*':
				case '?':
				case '.':
					continue;	
				default:
					break;
			}
			ExpNode nextNode(x,y,this, current.moves+1);
			bool found = false;
			for (std::vector<ExpNode>::iterator iter = closed.begin(); iter != closed.end(); ++iter) {
				if (nextNode == *iter) {
					found = true;
					break;
				}
			}
			if (!found) open.push(nextNode);
		}
	}
	
	return 0;
}

char World::findTile(char target){
	for(int i = seenXMin; i <= seenXMax; i++){
		for(int j = seenYMin; j <= seenYMax; j++){
			if(getMap(i,j) == target){
				return aStar(i,j);
			}
		}
	}
	return 0;
}

void World::print() const {
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

char World::getFront() const {
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

char getAction(World &world) {
	world.print();
	printf("%d %d", world.getPositionX(), world.getPositionY());
	
	// Plan:
	// If gold can be accessed by walking/boat, then access it and return gold
	// TODO
	// Otherwise, explore via walking/boat, chop trees if possible
	char move = 0;
	if(world.getGold()){
		move = world.aStar(0,0);
	}
	else{
		move = world.explore();
		if (move == 0){
			move = world.findTile('g');
		}
		if (move == 0){
			if(world.getAxe()){
				move = world.findTile('T');
			}
			else {
				move = world.findTile('a');
			}
		}
	}
	// If completely explored, then floodfill map with lowest number of bombs required to access a coordinate
	// TODO
	// Attempt to use bombs to access gold/tools in most cost effective manner
	// TODO
	// Repeat from top
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
		// scan 5-by-5 window around current location
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
