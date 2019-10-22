/***************************************************
* ESP32 Badge "Tombstone City: 21st Century" game
* An emulation of the classic TI-994/A game by John Plaster for Texas Instruments 
* *
* Requires:
* - ESP32 Devkit v1 badge
****************************************************/

#include "esp32_badge.h"
#include "Tombstone_title.h" 
#include "Tiles.h" 
#include "Audio.h"

// ---- Game Settings and Variables ----

#define DIRT_COLOR  0xFEF1
#define BROWN_COLOR 0x7980

#define TILE_SIZE      10
#define TITLE_HT       30
#define TITLE_WD       280
#define BORDER_TOP     10
#define BORDER_LEFT    20
#define BORDER_BOTTOM  SCREEN_HT - 10
#define BORDER_RIGHT   SCREEN_WD - 20
#define STATUS_TOP     GAME_BOTTOM  //BORDER_BOTTOM - 20
#define STATUS_HT      20
#define GAME_ROWS      17
#define GAME_COLS      28
#define GAME_TOP       BORDER_TOP + TITLE_HT
#define GAME_LEFT      BORDER_LEFT
#define GAME_BOTTOM    GAME_TOP + GAME_ROWS * TILE_SIZE
#define GAME_RIGHT     GAME_LEFT + GAME_COLS * TILE_SIZE
#define GAME_HT        GAME_BOTTOM - GAME_TOP
#define GAME_WD        GAME_RIGHT - GAME_LEFT
#define MAX_SPEED      20  // for morg or tumbleweed movement, higher value makes game easier


enum tile_type {
  TILE_UNKNOWN,
  TILE_EMPTY,
  TILE_PLAYER,
  TILE_SHOT,
  TILE_BLOCK,
  TILE_CACTUS,
  TILE_CACTUS_ACTIVE,
  TILE_TUMBLEWEED,
  TILE_MORG,
  TILE_EXPLOSION
};

enum game_state_type {
  STATE_TITLE,
  STATE_RULES,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_GAME_OVER
};

enum direction_type {
  DIR_UP,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
};

enum game_state_type game_state;
static int      difficulty, day, population, schooners;
static uint8_t  player_row, player_col, player_dir;
static uint8_t  shot_row, shot_col;
static int      shot_row_chg, shot_col_chg;
static bool     shot_active, shot_is_new;
static uint8_t  game_grid[GAME_ROWS * GAME_COLS];
static uint16_t spawn_counter, spawn_signal, spawn_max;
static int      spawn_row, spawn_col;
static uint8_t  morg_speed, tumbleweed_speed;

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

// ---- Function Prototypes ----

void    drawBackground();
void    drawTitleScreen();
void    drawRulesScreen();
void    drawGameScreen();

bool    isOutOfBounds(int row, int col);
bool    isInBounds(int row, int col, bool allowInCity);
bool    isOccupied(int row, int col);
uint8_t getTileType(int row, int col);
void    setTileType(int row, int col, uint8_t tile, bool draw);


/*
 * Initialize the board hardware and software on startup
 */
void setup() 
{
  Serial.begin(9600);
  Serial.println("ESP32 Badge - Tombstone City"); 
  delay(100);

  ledcSetup(spkr_channel, 12000, 8);  // 12 kHz max, 8 bit resolution
  ledcAttachPin(SPKR, spkr_channel);

  // Set up user input pins
  // Pins 24, 35, 36, & 39 do NOT have internal pullups, so need 4k7 physical resistors for these
  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);   
  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BTN_A, INPUT_PULLUP);
  pinMode(BTN_B, INPUT_PULLUP);
  pinMode(BTN_X, INPUT_PULLUP);
  pinMode(BTN_Y, INPUT_PULLUP);

  // Set up the speaker and LED output pins
  pinMode(SPKR, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(LED_3, OUTPUT);

  delay(100);
 
  tft.begin();
  tft.setRotation(SCREEN_ROT);
  delay(100);

  initGame();
  drawBackground();
  drawTitleScreen();
}

/*
 * Draws the dirt-colored background used in the game other screens
 */
void drawBackground()
{
  tft.fillScreen(BROWN_COLOR);
  tft.drawRGBBitmap(BORDER_LEFT, BORDER_TOP, (uint16_t *)tombstone_title, TITLE_WD, TITLE_HT);
}

/*
 * Displays the title/options screen
 */
void drawTitleScreen()
{
  tft.fillRect(GAME_LEFT, GAME_TOP, GAME_WD, GAME_HT + STATUS_HT, DIRT_COLOR);
  
  tft.setTextColor(ILI9341_BLACK);  
  tft.setTextSize(2);
  tft.setCursor(40, 80);
  tft.print("[A] LEVEL 1 = NOVICE");
  tft.setCursor(40, 100);
  tft.print("[B] LEVEL 2 = MASTER");
  tft.setCursor(40, 120);
  tft.print("[X] LEVEL 3 = INSANE");

  tft.setCursor(90, 160);
  tft.print("YOUR CHOICE?");

  tft.setCursor(44, 200);
  tft.print("PRESS [Y] FOR RULES");

  game_state = STATE_TITLE;
}

/*
 * Displays the player rules and information screen.
 */
void drawRulesScreen()
{
  tft.fillRect(GAME_LEFT, GAME_TOP, GAME_WD, GAME_HT + STATUS_HT, DIRT_COLOR);
  
  tft.setTextColor(ILI9341_BLACK);  
  tft.setTextSize(2);
  tft.setCursor(60, 50);
  tft.print("MOVE");
  tft.setCursor(190, 50);
  tft.print("DIR BTNS");
  
  tft.setCursor(60, 70);
  tft.print("FIRE");
  tft.setCursor(190, 70);
  tft.print("[A]-[B]");

  tft.drawRGBBitmap(44, 104, (uint16_t *)cactus_tile, 10, 10);
  tft.setCursor(60, 100);
  tft.print("SAGUARO");
  tft.setCursor(190, 100);
  tft.print("  0 PTS");

  tft.drawRGBBitmap(44, 124, (uint16_t *)tumbleweed_tile, 10, 10);
  tft.setCursor(60, 120);
  tft.print("TUMBLEWEED");
  tft.setCursor(190, 120);
  tft.print("100 PTS");

  tft.drawRGBBitmap(44, 144, (uint16_t *)morg1_tile, 10, 10);
  tft.setCursor(60, 140);
  tft.print("MORG");
  tft.setCursor(190, 140);
  tft.print("150 PTS");

  tft.setCursor(60, 170);
  tft.print("PAUSE GAME");
  tft.setCursor(234, 170);
  tft.println("[X]");

  tft.setCursor(60, 190);
  tft.print("PANIC BUTTON");
  tft.setCursor(234, 190);
  tft.print("[Y]");

  //tft.fillRect(160, 60, 4, 10, BROWN_COLOR);

  tft.setCursor(36, 210);
  tft.print("PRESS [Y] TO CONTINUE");

  game_state = STATE_RULES;
}

/*
 * Displays the game screen and status area
 */
void drawGameScreen() 
{ 
  // Clear out the entire game screen area
  tft.fillRect(GAME_LEFT, GAME_TOP, GAME_WD, GAME_HT + STATUS_HT, DIRT_COLOR);

  drawCity(true);

  // status area separator bar
  tft.fillRect(BORDER_LEFT, STATUS_TOP, GAME_WD, 4, ILI9341_BLACK);

  tft.setTextColor(ILI9341_BLACK);  
  tft.setTextSize(1);
  
  tft.setCursor(GAME_LEFT + 10, STATUS_TOP + 8);
  tft.print("DAY");
  drawDay();

  tft.setCursor(GAME_LEFT + 80, STATUS_TOP + 8);
  tft.print("POPULATION");
  drawPopulation();

  tft.setCursor(GAME_LEFT + 200, STATUS_TOP + 8);
  tft.print("SCHOONERS");
  drawSchooners();

  addCacti(10 + difficulty * 5);  
  addSpawnCacti(difficulty); // ensure that we have at least one spawning pair
  
  addTumbleweeds(10 + 5 * difficulty);  // testing
  addMorgs(difficulty);  

  playerStart();

  game_state = STATE_PLAYING;
}

/*
 * Updates the displayed Day value (aka the Wave #)
 */
void drawDay() 
{
  tft.setTextColor(ILI9341_BLACK);  
  tft.fillRect(GAME_LEFT + 10, STATUS_TOP + 18, 60, 10, DIRT_COLOR);
  tft.setCursor(GAME_LEFT + 10, STATUS_TOP + 18);
  tft.print(day);
}

/*
 * Updates the displayed Population value (aka the Score)
 */
void drawPopulation() 
{
  tft.setTextColor(ILI9341_BLACK);  
  tft.fillRect(GAME_LEFT + 80, STATUS_TOP + 18, 60, 10, DIRT_COLOR);
  tft.setCursor(GAME_LEFT + 80, STATUS_TOP + 18);
  tft.print(population);
}

/*
 * Updates the displayed Schooners value (aka the Player Lives remaining)
 */
void drawSchooners() 
{
  tft.setTextColor(ILI9341_BLACK);  
  tft.fillRect(GAME_LEFT + 200, STATUS_TOP + 18, 60, 10, DIRT_COLOR);
  tft.setCursor(GAME_LEFT + 200, STATUS_TOP + 18);
  tft.print(schooners);
}

/*
 * Displays the PAUSED message at the top of the screen and waits until Y is pressed again
 */
void drawPaused()
{
  tft.fillRect(GAME_LEFT + 80, GAME_TOP - 20, 112, 20, ILI9341_CYAN);
  tft.setCursor( GAME_LEFT + 100, GAME_TOP - 14);
  tft.print("** PAUSED **");
  
  game_state = STATE_PAUSED;
  delay(100);
}

/*
 * Erases the PAUSED message at the top of the screen and goes back to normal game playing
 */
void erasePaused()
{
  tft.drawRGBBitmap(BORDER_LEFT, BORDER_TOP, (uint16_t *)tombstone_title, TITLE_WD, TITLE_HT);
  game_state = STATE_PLAYING;
}

/*
 * That's all folks - draw the GAME OVER message
 */
void drawGameOver()
{
  tft.fillRect(GAME_LEFT + 80, GAME_TOP + 80, 100, TILE_SIZE, DIRT_COLOR);
  tft.setCursor( GAME_LEFT + 110, GAME_TOP + 81);
  tft.print("GAME OVER");
  
  playThemeSong();

  tft.fillRect(GAME_LEFT + 80, GAME_TOP + 100, 100, TILE_SIZE, DIRT_COLOR);
  tft.setCursor( GAME_LEFT + 70, GAME_TOP + 101);
  tft.print("PRESS [Y] TO RESTART");

  game_state = STATE_GAME_OVER;
}


/*
 * Displays the central 4x4 grid of blocks representing the city. 
 * When active, the blocks will have a light blue center, indicating that a spawn is pending.
 */
void drawCity(bool active)  // the center 4x4 grid of blue blocks
{
  for (int row = 5; row < 12; row+= 2)
  {
    for (int col = 10; col < 17; col+= 2)
    {
      tft.fillRect(GAME_LEFT + col * TILE_SIZE, GAME_TOP + row * TILE_SIZE, TILE_SIZE, TILE_SIZE, ILI9341_BLUE); 
      if (active)
      {
        tft.fillRect(GAME_LEFT + 2 + col * TILE_SIZE, GAME_TOP + 2 + row * TILE_SIZE, TILE_SIZE - 4, TILE_SIZE - 4, ILI9341_CYAN); 
      }
      setTileType(row, col, TILE_BLOCK, false);
    }
  }
}

/*
 * Displays the player Schooner at it's current location and direction.
 */
void drawPlayer()  
{
  switch(player_dir)
  {
    case DIR_UP:
      tft.drawRGBBitmap(player_col * TILE_SIZE + GAME_LEFT, player_row * TILE_SIZE + GAME_TOP, (uint16_t *)player_up_tile, TILE_SIZE, TILE_SIZE); break;
    case DIR_DOWN:
      tft.drawRGBBitmap(player_col * TILE_SIZE + GAME_LEFT, player_row * TILE_SIZE + GAME_TOP, (uint16_t *)player_down_tile, TILE_SIZE, TILE_SIZE); break;
    case DIR_LEFT:
      tft.drawRGBBitmap(player_col * TILE_SIZE + GAME_LEFT, player_row * TILE_SIZE + GAME_TOP, (uint16_t *)player_left_tile, TILE_SIZE, TILE_SIZE); break;
    case DIR_RIGHT:
      tft.drawRGBBitmap(player_col * TILE_SIZE + GAME_LEFT, player_row * TILE_SIZE + GAME_TOP, (uint16_t *)player_right_tile, TILE_SIZE, TILE_SIZE); break;
    default:
      tft.drawRGBBitmap(player_col * TILE_SIZE + GAME_LEFT, player_row * TILE_SIZE + GAME_TOP, (uint16_t *)player_up_tile, TILE_SIZE, TILE_SIZE); 
  }
}

/*
 * Moves the player in the requested direction.  Also handles enemy collisions and obstacle checking.
 */
void movePlayer(uint8_t moveDirection, int rowChg, int colChg)
{
  if (moveDirection != player_dir)
  {
    // just have player face in the new direction
    player_dir = moveDirection;
    drawPlayer();
    return;
  }

  if (isOutOfBounds(player_row + rowChg, player_col + colChg))
    return;

  uint8_t tile = getTileType(player_row + rowChg, player_col + colChg);
 
  if (tile == TILE_EMPTY)
  {
    setTileType(player_row, player_col, TILE_EMPTY, true);
    player_row += rowChg;
    player_col += colChg;
    drawPlayer();
    setTileType(player_row, player_col, TILE_PLAYER, false);
    return;
  }
   
  if (tile == TILE_MORG)
  {
      killPlayer();
      return;
  }
  
}

/*
 * Handles pathfinding logic for the morg at the specified game_grid index.
 * Morgs always try to move towards the player.
 */
void moveMorg(int gridIndex)
{
  uint8_t row, col;
  uint8_t move_row, move_col;

  if (random(MAX_SPEED) > morg_speed)
    return; // don't move
  
  row = gridIndex / GAME_COLS;
  col = gridIndex % GAME_COLS;
  move_row = row;
  move_col = col; 

  if (player_col > col)
    move_col = col + 1;

  if (player_col < col)
    move_col = col - 1;

  if (player_row > row)
    move_row = row + 1;

  if (player_row < row)
    move_row = row - 1;

  if ((move_row == player_row && col == player_col) || (row == player_row && move_col == player_col))
  {
    killPlayer();
    return;
  }

  if (move_row == row && move_col == col)
    return;  // not trying to move
    
  if (!isInCity(move_row, col) && !isOccupied(move_row, col))
  {
      setTileType(row, col, TILE_EMPTY, true);
      setTileType(move_row, col, TILE_MORG, true);
      return;
  }

  if (!isInCity(row, move_col) && !isOccupied(row, move_col))
  {
      setTileType(row, col, TILE_EMPTY, true);
      setTileType(row, move_col, TILE_MORG, true);
      return;
  }
}

/*
 * Handles movement logic for the tumbleweed at the specified game_grid index.
 * Tumbleweeds always try to run away from the player.
 */
void moveTumbleweed(int gridIndex)
{
  uint8_t row, col;
  uint8_t move_row, move_col;

  if (random(MAX_SPEED) > tumbleweed_speed)
    return; // don't move
  
  row = gridIndex / GAME_COLS;
  col = gridIndex % GAME_COLS;
  move_row = row;
  move_col = col; 

  if (player_row == row && player_col > col && col > 0)
    move_col = col - 1;

  if (player_row == row && player_col < col && col < GAME_COLS - 1)
    move_col = col + 1;

  if (player_col == col && player_row > row && row > 0)
    move_row = row - 1;

    if (player_col == col && player_row < row && row < GAME_ROWS - 1)
    move_row = row + 1;

    if (isOccupied(move_row, move_col) || isInCity(move_row, move_col))
      return;

    if (move_row != row || move_col != col)
    {
      setTileType(row, col, TILE_EMPTY, true);
      setTileType(move_row, move_col, TILE_TUMBLEWEED, true);
    }
}

/*
 * Initiates movement checks for all movement-capable tiles in the game grid (morgs and tumbleweeds)
 */
void doMovement()
{
  // Initialize the game tile grid
  uint8_t row, col;
  
  for (int i = 0; i < GAME_ROWS * GAME_COLS; i++)
  {
    if (game_grid[i] == TILE_TUMBLEWEED)
    {
      moveTumbleweed(i);
    }
    if (game_grid[i] == TILE_MORG)
    {
      moveMorg(i);
    }
  }
}

/*
 * Returns the game_grid index of a matching tile_type within 1 space of the (row, col) location.
 * Returns -1 if no matching neighbor is found.
 */
int getNeighborTile(int row, int col, uint8_t tile, bool allowInCity)
{
  // check clockwise starting from top
  int check_row = row - 1;
  int check_col = col;

  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // top right
  check_row = row - 1;
  check_col = col + 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // right
  check_row = row;
  check_col = col + 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // bottom right
  check_row = row + 1;
  check_col = col + 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // bottom
  check_row = row + 1;
  check_col = col;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // bottom left
  check_row = row + 1;
  check_col = col - 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // left
  check_row = row;
  check_col = col - 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  // top left
  check_row = row - 1;
  check_col = col - 1;
  if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
    return check_row * GAME_COLS + check_col;

  return -1;  // no matching neighbor found
}


/*
 * Returns the game_grid index of a matching tile_type within 1 space of the (row, col) location.
 * Returns -1 if no matching neighbor is found.
 */
int getNeighborTileEx(int row, int col, uint8_t tile, bool allowInCity, int excludeRow, int excludeCol)
{
  // check clockwise starting from top
  int check_row = row - 1;
  int check_col = col;

  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // top right
  check_row = row - 1;
  check_col = col + 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // right
  check_row = row;
  check_col = col + 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // bottom right
  check_row = row + 1;
  check_col = col + 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // bottom
  check_row = row + 1;
  check_col = col;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // bottom left
  check_row = row + 1;
  check_col = col - 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // left
  check_row = row;
  check_col = col - 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  // top left
  check_row = row - 1;
  check_col = col - 1;
  if (check_row != excludeRow || check_col != excludeCol)
  {
    if (isInBounds(check_row, check_col, allowInCity) && getTileType(check_row, check_col) == tile)
      return check_row * GAME_COLS + check_col;
  }
  
  return -1;  // no matching neighbor found
}


/*
 * Spawns a morg from the currently-active spawn cactus (as long as there is an open space around that cactus)
 */
void spawnMorg()
{
  int idx = getNeighborTile(spawn_row, spawn_col, TILE_EMPTY, false);
  if (idx >= 0)
  {
    int row = idx / GAME_COLS;
    int col = idx % GAME_COLS;
    setTileType(row, col, TILE_MORG, true);
  }

  setTileType(spawn_row, spawn_col, TILE_CACTUS, true);
}

/* 
 * Determines the next spawn point cactus location
 * sets the spawn_row and spawn_col values
 */
void prepareSpawn()
{
  bool chosen = false;
  int idx = -1;
  uint8_t count = random(1, 20);
  
  for (uint8_t i = 0; i < count; i++)
  {
    do 
    {
      idx = (idx + 1) % (GAME_ROWS * GAME_COLS);
      if (game_grid[idx] == TILE_CACTUS || game_grid[idx] == TILE_CACTUS_ACTIVE)
      {
        spawn_row = idx / GAME_COLS;
        spawn_col = idx % GAME_COLS;
      }
    } while (getNeighborTile(spawn_row, spawn_col, TILE_CACTUS, false) < 0);
  }

  setTileType(spawn_row, spawn_col, TILE_CACTUS_ACTIVE, true); // draw spawner
  setTileType(spawn_row, spawn_col, TILE_CACTUS, false); // simplifies cactus detection logic
}


/* 
 *  Handles the spawn timing and events.  Called every game cycle.
 */
void doSpawning()
{
  spawn_counter = (spawn_counter + 1) % spawn_max;

  if (spawn_counter == 0)
  {
    spawnMorg();
    drawCity(false);
    return;
  }

  if (spawn_counter == spawn_signal)
  {
    prepareSpawn();
    drawCity(true);
  }
}

/*
 * Returns the number of spawn-eligible cacti on the game grid.
 */
int getSpawnCount()
{
  int count = 0;
  int row, col, idx;
  for (int i = 0; i < GAME_ROWS * GAME_COLS; i++)
  {
    if (game_grid[i] == TILE_CACTUS)
    {
      row = i / GAME_COLS;
      col = i % GAME_COLS;
      if (getNeighborTile(row, col, TILE_CACTUS, false) >= 0)
      {
        // this cactus has a cactus neighbor
        count += 1;
      }
    }
  }
  return count;
}

/*
 * Checks for 3 cacti next to each other after a morg is killed
 */
void checkCactusTriplet(int row, int col)
{
  int idx1 = getNeighborTile(row, col, TILE_CACTUS, false);
  if (idx1 < 0)
    return; // no cactus neighbor

  int row1 = idx1 / GAME_COLS;
  int col1 = idx1 % GAME_COLS;

  int idx2 = getNeighborTileEx(row1, col1, TILE_CACTUS, false, row, col);

  if (idx2 < 0)
  {
    // that cactus doesn't have a neighbor, so see if original cactus has two neighbors
    idx2 = getNeighborTileEx(row, col, TILE_CACTUS, false, row1, col1);
  }

  if (idx2 < 0)
    return; // no triplet present

  int row2 = idx2 / GAME_COLS;
  int col2 = idx2 % GAME_COLS;

  // two cacti go bye, third becomes a morg
  setTileType(row, col, TILE_EMPTY, true);
  setTileType(row1, col1, TILE_EMPTY, true);
  setTileType(row2, col2, TILE_MORG, true);

  if (getSpawnCount() < 1)
  {
    advanceDay();  
  }
}

/*
 * Plays the player shot sound
 */
void playShot()
{
  ledcWrite(spkr_channel, 128);

  for (double d = 1000; d > 200; d=d-20)
  {
    ledcWriteTone(spkr_channel, d);
    delay(2);
  }
  delay(20);
  ledcWrite(spkr_channel, 0);
}

/* 
 *  Plays the explosion sound when something is destroyed
 */
void playExplosion()
{
  ledcWrite(spkr_channel, 128);
  
  for (double d = 600; d > 100; d=d-40)
  {
    ledcWriteTone(spkr_channel, d);
    delay(10);
  }
  delay(20);
  ledcWrite(spkr_channel, 0);
}

/* 
 *  Plays the teleport sound when player hits the panic button
 */
void playTeleport()
{
  ledcWrite(spkr_channel, 128);
  
  for (double d = 100; d < 1000; d=d+25)
  {
    ledcWriteTone(spkr_channel, d);
    delay(10);
  }
  delay(20);
  ledcWrite(spkr_channel, 0);
}

/*
 * Plays the next day theme
 */
void playNextDaySong()
{
  ledcWrite(spkr_channel, 128);
  
  ledcWriteTone(spkr_channel, F5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, Ds5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, G5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, As5);
  delay(WHOLE_NOTE);

  ledcWrite(spkr_channel, 0);
}

/*
 * Plays the Tombstone City theme song
 */
void playThemeSong()
{
  ledcWrite(spkr_channel, 128);
  
  ledcWriteTone(spkr_channel, As5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, C6);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, As5);
  delay(WHOLE_NOTE);
  ledcWriteTone(spkr_channel, G5);
  delay(HALF_NOTE);
  ledcWriteTone(spkr_channel, F5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, Ds5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, G5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, As5);
  delay(WHOLE_NOTE);

  ledcWriteTone(spkr_channel, C5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, Ds5);
  delay(HALF_NOTE);
  ledcWriteTone(spkr_channel, F5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, Ds5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, F5);
  delay(QTR_NOTE);
  ledcWriteTone(spkr_channel, G5);
  delay(WHOLE_NOTE + WHOLE_NOTE);

  ledcWrite(spkr_channel, 0);
}


/* 
 * Player dies when hit by a Morg 
 */
void killPlayer()
{ 
  setTileType(player_row, player_col, TILE_EXPLOSION, true); 
  playExplosion();
  
  setTileType(player_row, player_col, TILE_EMPTY, true);
  if (shot_active)
    setTileType(shot_row, shot_col, TILE_EMPTY, true);
  
  delay(200);

  if (schooners == 0)
  {
    drawGameOver();
    return;    
  }

  schooners -= 1;
  drawSchooners();
  playerStart();
}

/*
 * Handles creation of a new player shot.  Called whenever the fire button is pressed.
 */
void playerShoot()
{
  if (shot_active)
    return;
  
  playShot();
  
  shot_row = player_row;
  shot_col = player_col;
  shot_row_chg = 0;
  shot_col_chg = 0;
  shot_active = true;
  shot_is_new = true;

  switch(player_dir)
  {
    case DIR_UP:
      shot_row_chg = -1; break;
    case DIR_DOWN:
      shot_row_chg = 1; break;
    case DIR_LEFT:
      shot_col_chg = -1; break;
    case DIR_RIGHT:
      shot_col_chg = 1; break;
    default:
      shot_active = false;
  }
}

/*
 * Teleport schooner to center of city.  Subtracts 1000 populuation.
 */
void playerPanic()
{
  if (population < 1000 || isInCity(player_row, player_col))
    return;  // can't teleport

  setTileType(player_row, player_col, TILE_EMPTY, true);
  playTeleport();
    
  player_row = 8;
  player_col = 13;
  drawPlayer();
  delay(100);
}

/*
 * Moves the player shot and detects any collisions.  Called every game cycle.
 */
void moveShot()
{
  if (!shot_active)
    return;

  if (isOutOfBounds(shot_row + shot_row_chg, shot_col + shot_col_chg))
  {
    shot_active = false;
    if (!shot_is_new)
      setTileType(shot_row, shot_col, TILE_EMPTY, true);
    return;
  } 

  uint8_t tile = getTileType(shot_row + shot_row_chg, shot_col + shot_col_chg);

  if (tile == TILE_EMPTY)
  {
    if (!shot_is_new)
      setTileType(shot_row, shot_col, TILE_EMPTY, true); // erase old
    shot_row += shot_row_chg;
    shot_col += shot_col_chg;
    setTileType(shot_row, shot_col, TILE_SHOT, true);  // set new
    return;
  }

  if (tile == TILE_TUMBLEWEED)
  {
    if (!shot_is_new)
      setTileType(shot_row, shot_col, TILE_EMPTY, true);  // erase shot

    setTileType(shot_row + shot_row_chg, shot_col + shot_col_chg, TILE_EXPLOSION, true); 
    playExplosion();
    
    shot_active = false;
    population += 100;
    drawPopulation();
    delay(50);
    
    setTileType(shot_row + shot_row_chg, shot_col + shot_col_chg, TILE_EMPTY, true); // erase tumbleweed
    return;
  }

  if (tile == TILE_MORG)
  {
    if (!shot_is_new)
      setTileType(shot_row, shot_col, TILE_EMPTY, true);  // erase shot

    setTileType(shot_row + shot_row_chg, shot_col + shot_col_chg, TILE_EXPLOSION, true); 
    playExplosion();
    
    shot_active = false;
    population += 150;
    drawPopulation();
    delay(50);
    
    setTileType(shot_row + shot_row_chg, shot_col + shot_col_chg, TILE_CACTUS, true); // dead morg becomes cactus
    checkCactusTriplet(shot_row + shot_row_chg, shot_col + shot_col_chg);
    return;
  }

  // hit something else (cactus or city)
  shot_active = false;
  if (!shot_is_new)
      setTileType(shot_row, shot_col, TILE_EMPTY, true);  // erase shot
  
}

/*
 * Adds the number of new cacti to random locations in the game area specified in the count value.
 */
void addCacti(uint8_t count)
{
  long row, col;
  for (uint8_t i = 0; i < count; i++)
  {
    do 
    {
      row = random(GAME_ROWS);
      col = random(GAME_COLS);
    } while (isOccupied(row, col) || isInCity(row, col)); 
    
    setTileType(row, col, TILE_CACTUS, true);
  }
}

/*
 * Adds a new cactus to an existing solitary cactus it finds in order to create the specified count of spawning pairs.
 */
void addSpawnCacti(uint8_t count)
{
  int row, col, idx;
  for (int cnt = 0; cnt < count; cnt++)
  {
    for (int i = 0; i < GAME_ROWS * GAME_COLS; i++)
    {
      if (game_grid[i] == TILE_CACTUS)
      {
        row = i / GAME_COLS;
        col = i % GAME_COLS;
        if (getNeighborTile(row, col, TILE_CACTUS, false) < 0)
        {
          // this cactus has no cactus neighbors
          idx = getNeighborTile(row, col, TILE_EMPTY, false);
          if (idx >=0)
          {
            row = idx / GAME_COLS;
            col = idx % GAME_COLS;
            setTileType(row, col, TILE_CACTUS, true);
            break;
          }
        }
      }
    }
  }
}

/*
 * Adds the number of new tumbleweeds to random locations in the game area specified in the count value.
 */
void addTumbleweeds(uint8_t count)
{
  long row, col;
  for (uint8_t i = 0; i < count; i++)
  {
    do 
    {
      row = random(GAME_ROWS);
      col = random(GAME_COLS);
    } while (isOccupied(row, col) || isInCity(row, col)); 
    
    setTileType(row, col, TILE_TUMBLEWEED, true);
  }
}

/*
 * Adds the number of new morg enemies to random locations in the game area specified in the count value.
 */
void addMorgs(uint8_t count)
{
  long row, col;
  for (uint8_t i = 0; i < count; i++)
  {
    do 
    {
      row = random(GAME_ROWS);
      col = random(GAME_COLS);
    } while (isOccupied(row, col) || isInCity(row, col)); 
    
    setTileType(row, col, TILE_MORG, true);
  }
}


/*
 * Determines if the (row, col) location is outside of the allowable game tile region
 */
bool isOutOfBounds(int row, int col)
{
  if (row < 0 || row > GAME_ROWS - 1 || col < 0 || col > GAME_COLS - 1)
    return true;
  else
    return false;
}

/*
 * Determines if the (row, col) location is insided of the allowable game tile region.
 * If allowInCity is false, also makes sure the location is not inside the central city grid area.
 */
bool isInBounds(int row, int col, bool allowInCity)
{
  if (!isOutOfBounds(row, col))
  {
    if (allowInCity)
      return true;
    else
      return !isInCity(row, col);
  }
}

/*
 * Determines if the give (row, col) coordinates are within the central city grid area.
 */
bool isInCity(int row, int col)
{
  if (row >= 5 && row <= 11 && col >= 10 && col <= 16)
    return true;
  else
    return false;
}

/*
 * Returns true if the specified (row, col) location has any value other than the ground tile_type.
 */
bool isOccupied(int row, int col)
{
  if (isOutOfBounds(row, col))
    return true;

  if (getTileType(row, col) == TILE_EMPTY)
    return false;
  else
    return true;
}

/*
 * Converts the (row, col) location to a game grid index
 */
uint8_t getGridIndex(int row, int col)
{
    return row * GAME_COLS + col;
}

/*
 * Returns the tile_type at the specified (row, col) game grid location.
 */
uint8_t getTileType(int row, int col)
{
  if (!isOutOfBounds(row, col))
  {
    return game_grid[row * GAME_COLS + col];
  }
  return TILE_UNKNOWN;
}

/*
 * Sets the the tile_type at the specified (row, col) game grid location.
 * Also optionally draws the new tile to the screen.
 */
void setTileType(int row, int col, uint8_t tile, bool draw)
{
  if (!isOutOfBounds(row, col))
  {
    game_grid[row * GAME_COLS + col] = tile;
    if (draw)
    {
      switch(tile)
      {
        case TILE_EMPTY:
          tft.fillRect(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, TILE_SIZE, TILE_SIZE, DIRT_COLOR); break;
        case TILE_SHOT:
          tft.fillRect(col * TILE_SIZE + GAME_LEFT + 3, row * TILE_SIZE + GAME_TOP + 3, 4, 4, ILI9341_BLACK); break;
        case TILE_BLOCK:
          tft.fillRect(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, TILE_SIZE, TILE_SIZE, ILI9341_BLUE); break;
        case TILE_CACTUS:
          tft.drawRGBBitmap(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, (uint16_t *)cactus_tile, TILE_SIZE, TILE_SIZE); break;
        case TILE_CACTUS_ACTIVE:
          tft.drawRGBBitmap(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, (uint16_t *)cactus_active_tile, TILE_SIZE, TILE_SIZE); break;
        case TILE_TUMBLEWEED:
          tft.drawRGBBitmap(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, (uint16_t *)tumbleweed_tile, TILE_SIZE, TILE_SIZE); break;
        case TILE_MORG:  
          tft.drawRGBBitmap(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, (uint16_t *)morg1_tile, TILE_SIZE, TILE_SIZE); break;
        case TILE_EXPLOSION:  
          tft.drawRGBBitmap(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, (uint16_t *)explosion_tile, TILE_SIZE, TILE_SIZE); break;
        default:
          tft.fillRect(col * TILE_SIZE + GAME_LEFT, row * TILE_SIZE + GAME_TOP, TILE_SIZE, TILE_SIZE, DIRT_COLOR); break;
      }
    }
  }
}



/*
 * Sets the TFT text color and optionally inverts the text.  Assumes a black background color.
 */
void setTextColor(uint16_t color, bool inverted)
{
  if (inverted)
    tft.setTextColor(ILI9341_BLACK, color);
  else
    tft.setTextColor(color, ILI9341_BLACK);
  
}

/*
 * Initialize the game settings and grid at the start of a new game.
 */
void initGame()
{
  day = 1;
  population = 0;
  schooners = 3;
  
  morg_speed = 2;  
  tumbleweed_speed = 2;

  spawn_counter = 0;
  spawn_signal = 150;  // spawn is highlighted
  spawn_max = 200;

  // Initialize the game tile grid
  for (int i = 0; i < GAME_ROWS * GAME_COLS; i++)
  {
    game_grid[i] = TILE_EMPTY;
  }
}

/*
 * Initialize the player values at the start of a game or after the player dies.
 */
void playerStart()
{
  player_row = 8;
  player_col = 13;
  player_dir = DIR_UP;
  shot_active = false;

  setTileType(player_row, player_col, TILE_PLAYER, false);
  drawPlayer();
}

/*
 * Advances game to the next day (difficulty level increase)
 */
void advanceDay()
{
  day += 1; 
  schooners += 1;
  drawDay();
  drawSchooners();
  playNextDaySong();
  
  addCacti(day + difficulty);
  addSpawnCacti(day + difficulty);

  if (spawn_max > 50)
    spawn_max -= 25;

  spawn_signal = spawn_max - 50;
}

/*
 * The main program (game) loop.  Runs continuously after Setup()
 */
void loop(void) 
{
  if(digitalRead(BTN_UP) == LOW)
  {
    btnUp_pressed = true;
  }
  else
  {
    btnUp_pressed = false;
  }

  if(digitalRead(BTN_DOWN) == LOW)
  {
    btnDown_pressed = true;
  }
  else
  {
    btnDown_pressed = false;
  }

  if(digitalRead(BTN_LEFT) == LOW)
  {
    btnLeft_pressed = true;
  }
  else
  {
    btnLeft_pressed = false;
  }

  if(digitalRead(BTN_RIGHT) == LOW)
  {
    btnRight_pressed = true;
  }
  else
  {
    btnRight_pressed = false;
  }
  
  
  if(digitalRead(BTN_X) == LOW)
  {
    btnX_pressed = true;
  }
  else
  {
    btnX_pressed = false;
  }

  if(digitalRead(BTN_Y) == LOW)
  {
    btnY_pressed = true;
  }
  else
  {
    btnY_pressed = false;
  }

  if(digitalRead(BTN_A) == LOW)
  {
    btnA_pressed = true;
  }
  else
  {
    btnA_pressed = false;
  }
  
  if(digitalRead(BTN_B) == LOW)
  {
    btnB_pressed = true;
  }
  else
  {
    btnB_pressed = false;
  }

  switch(game_state)
  {
    case STATE_TITLE:
      handleTitle(); break;
    case STATE_RULES:
      handleRules();break;
    case STATE_PLAYING:
      handlePlaying();break;
    case STATE_PAUSED:
      handlePaused(); break;
    case STATE_GAME_OVER:
      handleGameOver(); break;
    
  }
  
  delay(50);
}

// ---- GAME STATE HANDLERS ----

/*
 * Handles the STATE_TITLE game state logic
 */
void handleTitle()
{
  if (btnA_pressed)
  {
    difficulty = 1;
    drawGameScreen();
    return;
  }

  if (btnB_pressed)
  {
    difficulty = 2;
    drawGameScreen();
    return;
  }

  if (btnX_pressed)
  {
    difficulty = 3;
    drawGameScreen();
    return;
  }

  if (btnY_pressed)
  {
    drawRulesScreen();
  }
}

/*
 * Handles the STATE_RULES game state logic
 */
void handleRules()
{
  if (btnY_pressed)
  {
    drawTitleScreen();
  }
}

/*
 * Handles the STATE_PLAYING game state logic
 */
void handlePlaying()
{
  if (btnUp_pressed)
    movePlayer(DIR_UP, -1, 0);
  else if(btnDown_pressed)
     movePlayer(DIR_DOWN, 1, 0);
  else if (btnLeft_pressed)
    movePlayer(DIR_LEFT, 0, -1);
  else if (btnRight_pressed)
    movePlayer(DIR_RIGHT, 0, 1);
    
  if (btnA_pressed)
    playerShoot();

  if (btnB_pressed)
  {
    playerShoot();
  }
  
  if (btnY_pressed)
    playerPanic();

  if (btnX_pressed)
  {
    drawPaused();
    return;
  }
  
  moveShot();
  shot_is_new = false;
  doMovement();
  doSpawning();
}

/*
 * Handles the STATE_PAUSED game state logic
 */
void handlePaused()
{
  if (btnX_pressed)
  {
   erasePaused();
  }
}

/*
 * Handles the STATE_GAME_OVER game state logic
 */
void handleGameOver()
{
  if (btnY_pressed)
  {
    drawTitleScreen();
  }
}
