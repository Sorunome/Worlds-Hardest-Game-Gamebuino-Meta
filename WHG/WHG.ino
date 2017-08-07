#include <Gamebuino-Meta.h>

#define MAPHEADER 3
#define MAPTILESIZE 6
#define TILETOPX(x) x*MAPTILESIZE + 1
#define MAPFLAGCOIN 0xFD
#define MAPFLAGENEMY 0xFE
#define MAPFLAGEND 0xFF

const byte logo[] = {64,36,0x00,0xF0,0x00,0x00,0x04,0x00,0x07,0xFC,0x01,0x28,0x00,0x20,0x0A,0x00,0x1F,0x80,0x8A,0x35,0x93,0x28,0x05,0x00,0x38,0xFF,0xAA,0xFD,0x52,0x90,0x32,0x00,0xEF,0xFE,0xAB,0xD5,0x92,0x88,0x68,0x01,0xFF,0xE0,0xDA,0x65,0x5B,0x30,0x94,0x07,0xF9,0x80,0x01,0x28,0x00,0x00,0x60,0x1F,0xE2,0x00,0xFF,0xFF,0xFF,0xE3,0x10,0x3F,0x84,0x00,0xAD,0x89,0x9A,0x22,0xC0,0x7E,0x18,0x00,0xA8,0xAA,0xB7,0x62,0xA0,0xF8,0x30,0x00,0x8A,0x9A,0x9B,0x49,0x01,0xF0,0x70,0x00,0xA8,0xAA,0xBB,0x50,0x81,0xE0,0xF8,0x00,0xAA,0xA9,0x97,0x56,0x01,0xC3,0x8C,0x00,0xFF,0xFF,0xFD,0xD2,0x03,0x87,0x06,0x00,0x00,0x00,0x00,0x0E,0x03,0x86,0x07,0x00,0x00,0x00,0x00,0x00,0x07,0x8F,0x0C,0x80,0x00,0x07,0xFF,0xFF,0x1B,0xFF,0x9A,0x40,0x00,0x0B,0x83,0x80,0x66,0x63,0xF5,0x20,0x00,0x0E,0xA3,0xFC,0xCC,0x88,0xEA,0x10,0x00,0x0C,0x82,0x11,0xD9,0x9C,0x65,0x8C,0x00,0x0F,0x93,0xFD,0xF1,0x0C,0x1B,0x42,0x00,0x08,0x2A,0x30,0xF3,0x7C,0x04,0xB1,0x00,0x0A,0x13,0x16,0xE6,0x0C,0x03,0x68,0x00,0x0E,0x02,0xCF,0x6C,0x5C,0x00,0x94,0x00,0x0A,0x06,0x38,0x19,0x5C,0x00,0x6B,0x00,0x0C,0x07,0x4E,0x42,0xAC,0x00,0x16,0x00,0x08,0xC6,0xA5,0x9A,0xAC,0x00,0x09,0x00,0x08,0xC2,0xA5,0x50,0x0C,0x00,0x06,0x00,0x0B,0x02,0x10,0x10,0x6C,0x00,0x01,0x00,0x0B,0x31,0xFF,0xE0,0x6C,0x00,0x00,0x00,0x04,0x30,0x00,0x01,0x8C,0x00,0x00,0x00,0x04,0xC6,0x00,0x71,0x9C,0x00,0x00,0x00,0x02,0xC9,0x55,0x70,0x38,0x00,0x00,0x00,0x01,0x87,0x00,0x21,0xF0,0x00,0x00,0x00,0x00,0xFF,0xFF,0xFF,0xE0,0x00,0x00,0x00,0x00,0x3F,0xFF,0xFF,0x00,0x00,0x00};
const byte ok[] = {8,7,0x2,0x4,0x88,0x48,0x50,0x30,0x20}; // from bub
const byte ko[] = {8,7,0x82,0x44,0x28,0x10,0x28,0x44,0x82}; // from bub

extern const byte *gamemaps[];
#define NUMLEVELS 22

byte playerX;
byte playerY;
int mapX = MAPTILESIZE;
int mapY = MAPTILESIZE;
byte numEnemies;
byte numCoins;
bool dead = false;
bool frameskip = false;
int tries;
byte winTile;
bool potentialWin = false;
const byte * gamemap;
const byte * spawnpoints;
byte curLevelNum;
byte curSavePoint;
byte mapWidth;
byte mapHeight;

class Coin {
	byte x, y;
	byte have;
	public:
		Coin(byte cx, byte cy) {
			x = cx;
			y = cy;
			have = 0;
		}
		void draw() {
			if (have > 0) {
				return;
			}
			if (gb.collideRectRect(x, y, 4, 4, playerX, playerY, 4, 4)) {
				have = 1;
				gb.sound.playTick();
			}
			int drawX = x + mapX;
			int drawY = y + mapY;
			if (drawX > 86 || drawY > 68 || drawX < -4 || drawY < -4) {
				return;
			}
			gb.display.setColor(YELLOW);
			gb.display.fillRect(drawX + 1, drawY + 1, 2, 2);
			gb.display.setColor(ORANGE);
			gb.display.drawRoundRect(drawX, drawY, 4, 4, 1);
		}
		void reset() {
			if (have != 2) {
				have = 0;
			}
		}
		void stick() {
			if (have == 1) {
				have = 2;
			}
		}
		bool doHave() {
			return have>0;
		}
};
Coin *coins[70];

class Enemy {
	int x, y;
	byte s;
	const byte* points;
	const byte* startPoints;
	byte nextPoint;
	public:
		Enemy(byte bs, const byte * ps) {
			x = ps[0];
			y = ps[1];
			s = bs;
			points = ps;
			startPoints = ps;
			nextPoint = 0;
		}
		void update() {
			if (s == 0) { // no need to update as we aren't moving
				return;
			}
			int nx = points[0];
			int ny = points[1];
			if (nx > x) {
				x += s;
				if (nx < x) {
					x = nx;
				}
			}
			if (nx < x) {
				x -= s;
				if (nx > x) {
					x = nx;
				}
			}
			
			if (ny > y) {
				y += s;
				if (ny < y) {
					y = ny;
				}
			}
			if (ny < y) {
				y -= s;
				if (ny > y) {
					y = ny;
				}
			}
			if (nx == x && ny == y) {
				points += 2;
				byte tmp = points[0];
				if (tmp == MAPFLAGENEMY || tmp == MAPFLAGCOIN || tmp == MAPFLAGEND) {
					points = startPoints;
				}
			}
		}
		void draw() {
			if (gb.collideRectRect(x, y, 4, 4, playerX, playerY, 4, 4)) {
				dead = true;
			}
			int drawX = x + mapX;
			int drawY = y + mapY;
			if (drawX > 86 || drawY > 68 || drawX < -4 || drawY < -4) {
				return;
			}
			gb.display.setColor(BLUE);
			gb.display.fillRect(drawX, drawY + 1, 4, 2);
			gb.display.fillRect(drawX + 1, drawY, 2, 4);
		}
};

Enemy *enemies[45];

void setup() {
	gb.begin();
	doTitleScreen(true);
}
void doTitleScreen(byte doTitle){
	do {
		destroyMap();
		if (doTitle) {
			gb.titleScreen(logo);
		}
		doTitle = true;
	} while(!doMainMenu());
}

void drawLevelMenu(byte curPick) {
	gb.display.setCursors(0, 0);
	gb.display.print("Level Menu");
	gb.display.print("\n\nLevel  \x11");
	if(curPick < 10){
		gb.display.print(" ");
	}
	gb.display.print(curPick);
	gb.display.print("\x10");
	
	byte done = gb.save.get(curPick - 1);
	if (done > 0) {
		gb.display.setColor(GREEN);
		gb.display.drawBitmap(48, 11, ok);
	} else {
		gb.display.setColor(RED);
		gb.display.drawBitmap(48, 11, ko);
	}
}
byte chooseLevel() {
	byte curPick = curLevelNum+1;
	while(1) {
		if (!gb.update()) {
			continue;
		}
		drawLevelMenu(curPick);
		if (gb.buttons.pressed(BUTTON_C)) {
			curLevelNum = curPick-1;
			gb.sound.playCancel();
			return NUMLEVELS;
		}
		if (gb.buttons.pressed(BUTTON_RIGHT)) {
			curPick++;
			if (curPick > NUMLEVELS) {
				curPick = NUMLEVELS;
			} else {
				gb.sound.playTick();
			}
		}
		if (gb.buttons.pressed(BUTTON_LEFT)) {
			curPick--;
			if (curPick < 1) {
				curPick = 1;
			} else {
				gb.sound.playTick();
			}
		}
		if (gb.buttons.pressed(BUTTON_A)) {
			gb.sound.playOK();
			break;
		}
	}
	return curPick - 1;
}

bool doMainMenu() {
	byte ret = chooseLevel();
	tries = 0;
	if (ret >= 0 && ret <= NUMLEVELS-1) {
		curLevelNum = ret;
		loadMap();
		return true;
	}
	return false;
}
void loop() {
	if (!gb.update()) {
		return;
	}
	potentialWin = false;
	if (gb.buttons.repeat(BUTTON_UP, 0)) {
		playerY--;
		if (getTileAtPos(0, 0) == 0 || getTileAtPos(3, 0) == 0) {
			playerY++;
		}
	}
	if (gb.buttons.repeat(BUTTON_DOWN, 0)) {
		playerY++;
		if (getTileAtPos(0, 3) == 0 || getTileAtPos(3, 3) == 0) {
			playerY--;
		}
	}
	if (gb.buttons.repeat(BUTTON_LEFT, 0)) {
		playerX--;
		if (getTileAtPos(0, 0)==0 || getTileAtPos(0, 3) == 0) {
			playerX++;
		}
	}
	if (gb.buttons.repeat(BUTTON_RIGHT, 0)) {
		playerX++;
		if (getTileAtPos(3, 0) == 0 || getTileAtPos(3, 3) == 0) {
			playerX--;
		}
	}
	drawWorld();
	if (dead) {
		tries++;
		gb.sound.playCancel();
		resetPlayer();
	} else if (potentialWin) {
		bool win = true;
		for (byte i = 0; i < numCoins; i++) {
			win &= coins[i]->doHave();
		}
		if (win) {
			gb.sound.playOK();
			gb.save.set(curLevelNum, 1);
			destroyMap();
			if (++curLevelNum >= NUMLEVELS) {
				curLevelNum--;
				doTitleScreen(false);
			} else {
				loadMap();
			}
		}
	}
	if (gb.buttons.pressed(BUTTON_C)) {
		gb.sound.playCancel();
		doTitleScreen(false);
	}
}
void loadMap(){
	gamemap = gamemaps[curLevelNum];
	numEnemies = 0;
	numCoins = 0;
	mapHeight = gamemap[MAPHEADER - 1];
	mapWidth = gamemap[MAPHEADER - 2];
	unsigned int i = MAPHEADER + (mapWidth * mapHeight);
	
	spawnpoints = gamemap+i;
	
	while (gamemap[i] != MAPFLAGEND) {
		byte flag = gamemap[i++]; // we want to increase i AFTER we check in either case
		if (flag == MAPFLAGENEMY) {
			enemies[numEnemies++] = new Enemy(gamemap[i++], gamemap + i);
		} else if (flag == MAPFLAGCOIN) {
			coins[numCoins++] = new Coin(gamemap[i++], gamemap[i]);
		}
	}
	winTile = gamemap[0];
	curSavePoint = 2;
	
	resetPlayer();
}
void destroyMap() {
	for (byte i = 0; i < numEnemies; i++) {
		delete enemies[i];
	}
	for (byte i = 0; i < numCoins; i++) {
		delete coins[i];
	}
	numEnemies = 0;
	numCoins = 0;
}
void resetPlayer() {
	dead = false;
	playerX = spawnpoints[0];
	playerY = spawnpoints[1];
	for (byte i = 0; i < numCoins; i++) {
		coins[i]->reset();
	}
}

byte getTileAtPos(byte xoffset, byte yoffset) {
	if (playerY+yoffset < 0 || playerX+xoffset < 0 || playerY+yoffset >= mapHeight*MAPTILESIZE || playerX+xoffset >= mapWidth*MAPTILESIZE) {
		return 0;
	}
	byte tile = gamemap[((playerY+yoffset) / MAPTILESIZE)*mapWidth + ((playerX+xoffset) / MAPTILESIZE) + MAPHEADER];
	potentialWin |= tile==winTile;
	if (tile > curSavePoint && !potentialWin) {
		spawnpoints += 2*(tile - curSavePoint);
		curSavePoint += tile - curSavePoint;
		for (byte i = 0; i < numCoins; i++) {
			coins[i]->stick();
		}
	}
	return tile;
}
void drawWorld() {
	if (playerX + mapX < 20) {
		mapX++;
	}
	if (playerX + mapX > 60) {
		mapX--;
	}
	if (playerY + mapY < 10) {
		mapY++;
	}
	if (playerY + mapY > 38) {
		mapY--;
	}
	gb.display.fillScreen(DARKGRAY);
	gb.display.setColor(WHITE);
	for (byte y = 0; y < mapHeight; y++) {
		for (byte x = 0; x < mapWidth; x++) {
			byte tile = gamemap[y*mapWidth + x + MAPHEADER];
			int drawX = x*MAPTILESIZE + mapX;
			int drawY = y*MAPTILESIZE + mapY;
			if (drawX > 86 || drawY > 68 || drawX < -4 || drawY < -4) {
				continue;
			}
			if (tile == 1) {
				if ((x%2 && !(y%2)) || (!(x%2) && y%2)) {
					gb.display.setColor(WHITE);
				} else {
					gb.display.setColor(BEIGE);
				}
				gb.display.fillRect(drawX, drawY, MAPTILESIZE, MAPTILESIZE);
			} else if (tile != 0) {
				gb.display.setColor(GREEN);
				gb.display.fillRect(drawX, drawY, MAPTILESIZE, MAPTILESIZE);
			}
			
		}
	}
	gb.display.setColor(RED);
	gb.display.fillRect(playerX + mapX + 1, playerY + mapY + 1, 2, 2);
	gb.display.setColor(BLACK);
	gb.display.drawRect(playerX + mapX, playerY + mapY, 4, 4);
	for (byte i = 0; i < numEnemies; i++) {
		if (!frameskip) {
			enemies[i]->update();
		}
		enemies[i]->draw();
	}
	for (byte i = 0; i < numCoins; i++) {
		coins[i]->draw();
	}
	gb.display.setCursors(1, 1);
	gb.display.setColor(WHITE, DARKGRAY);
	gb.display.print(tries);
	frameskip = !frameskip;
}
