/***************************************************
ESP32 Badge - Gravitack game
An homage to the Atari arcade console
by Paul Pagel

Requires:
 - ESP32 Badge
 ****************************************************/

#include "esp32_badge.h"
#include "XT_DAC_Audio.h";
#include "Levels.h"
#include "Bitmaps.h"
#include "FireWav.h"
#include "RefuelWav.h"
#include "ExplosionWav.h"
#include "WarpWav.h"
#include "TickWav.h"

#define SHIP_SIZE    8
#define THRUST_SIZE  3 
#define SHOT_SPEED   8
#define MAX_SHOTS    4
#define MAX_GROUND  32
#define MAX_FUEL     8
#define MAX_TURRET   8
#define MAX_LIVES    6

#define PLAYER_FUEL_X 30
#define GAME_TOP   20 
#define FUEL_WD     8
#define FUEL_HT    16
#define INVALID_ITEM 999

enum game_state_type {
  STATE_TITLE,
  STATE_RULES,
  STATE_PLAYING,
  STATE_PAUSED,
  STATE_GAME_OVER
};

enum direction_type {
  DIR_UP = 1,
  DIR_DOWN = 2,
  DIR_LEFT = 3,
  DIR_RIGHT = 4
};

enum game_state_type game_state;

// constants used to speed up trig calculations
const double PId2  = PI / 2;
const double PId15 = PI / 1.5;
const double PIx15 = PI * 1.5;
const double PIx075 = PI * 0.75;

static bool   btnB_pressed_prev;

static int    player_start_x, player_start_y;
static double player_ctr_x = SCREEN_WD / 2;
static double player_ctr_y = SCREEN_HT / 4;
static double player_rotation = PIx15;
static double player_speed_x;
static double player_speed_y;
static double player_fuel;
static int    player_lives;
static long   player_score;
static long   next_ship = 10000;
static double gravity = 0.02;
static int    level;
static int    level_delay = 10;

int ship_x[9];
int ship_y[9];
int shot_x[MAX_SHOTS];
int shot_y[MAX_SHOTS];
int shot_range[MAX_SHOTS];
double shot_speed_x[MAX_SHOTS];
double shot_speed_y[MAX_SHOTS];
int player_shot_range; // # of game loops new shots can exist

int ground_x[MAX_GROUND];
int ground_y[MAX_GROUND];
int ground_size = 0;  // # of ground line segments in the current level

int  fuel_x[MAX_FUEL];
int  fuel_y[MAX_FUEL];
bool fuel_active[MAX_FUEL];
int  fuel_count;    // # starting fuel packs in level
int  fuel_remaining;  // # remaining fuel packs in level

int  turret_x[MAX_TURRET];
int  turret_y[MAX_TURRET];
int  turret_dir[MAX_TURRET];
bool turret_active[MAX_TURRET];
int  turret_shot_x[MAX_TURRET];
int  turret_shot_y[MAX_TURRET];
int  turret_shot_range[MAX_TURRET];
double turret_shot_speed_x[MAX_TURRET];
double turret_shot_speed_y[MAX_TURRET];
int  turret_range;    // # of game loops new turret shots can exist
int  turret_count;    // # starting turrets in level
int  turret_remaining;  // # remaining turrets in level

int turret_offset_x[4] = { 10, 10,-10, 20};
int turret_offset_y[4] = {-10, 10, 10, 10};

// Sound FX
XT_DAC_Audio_Class DacAudio(SPKR, 0);  // main audio object
XT_Wav_Class fire_play(fire_wav);     
XT_Wav_Class refuel_play(refuel_wav); 
XT_Wav_Class explosion_play(explosion_wav); 
XT_Wav_Class warp_play(warp_wav);
XT_Wav_Class tick_play(tick_wav);

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_MOSI, TFT_CLK, TFT_RST, TFT_MISO);

/*
 *  Initialize hardware, peripherals, and libraries
 */
void setup() 
{
  Serial.begin(9600);
  Serial.println("ESP32 Badge - Gravitack"); 
  delay(100);

  // not needed since we'll be using digital audio, which does not share nicely with LEDC
  //ledcSetup(spkr_channel, 12000, 8);  // 12 kHz max, 8 bit resolution
  //ledcAttachPin(SPKR, spkr_channel);

  // Set up user input pins
  // Pins 24, 35, 36, & 39 on ESP32 do NOT have internal pullups, so need 4k7 physical resistors for these
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

  drawTitleScreen();
}

/*
 * Draws the game title/splash screen
 */
void drawTitleScreen()
{
  tft.fillScreen(ILI9341_BLACK);

  tft.drawRGBBitmap(20, 60, (uint16_t *)gravitack_title, 279, 77);
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(40, 200);
  tft.print("[X] RULES, [Y] START");

  game_state = STATE_TITLE;
}

/*
 * Draws the game instructions screen
 */
void drawRulesScreen()
{
  tft.fillScreen(ILI9341_BLACK);
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(50, 0);
  tft.print("CONTROLS & SCORING");

  tft.drawLine(0, 25, 320, 25, ILI9341_GREEN);

  tft.setTextColor(ILI9341_DARKGREY);
  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.print("Thrust: [A]");

  tft.setCursor(190, 40);
  tft.print("Pause: [X]");
  
  tft.setCursor(10, 60);
  tft.print("Fire:   [B]");
  
  
  tft.drawRect(24, 100, FUEL_WD, FUEL_HT, ILI9341_CYAN);
  
  tft.setCursor(50, 100);
  tft.print("200 pts");

  tft.setTextSize(1);
  tft.setCursor(160, 100);
  tft.print("Touch fuel packs with your");
  tft.setCursor(160, 110);
  tft.print("ship to pick them up");
  
  drawTurret(15, 140, 1);

  tft.setTextSize(2);
  tft.setCursor(50, 135);
  tft.print("500 pts"); 

  tft.setTextSize(1);
  tft.setCursor(160, 145);
  tft.print("Shoot all turrets");
  
  tft.setCursor(50, 165);
  tft.print("Pick up all fuel packs and destroy all");
  tft.setCursor(50, 175);
  tft.print("turrets to advance to the next level.");
  tft.setCursor(50, 190);
  tft.print("** Free ship every 10,000 points **");

  tft.drawLine(0, 210, 320, 210, ILI9341_GREEN);

  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(60, 220);
  tft.print("PRESS [Y] TO START");
  
  game_state = STATE_RULES;
}

/*
 * Initializes the game variables and draws the main gameplay screen
 */
void startGame(int startLevel)
{
  tft.fillScreen(ILI9341_BLACK);

  level = startLevel;
  player_score = 0;
  player_lives = 4;
  player_shot_range = 100;
  next_ship = 10000;
  turret_range = 10;

  loadLevel(level);
  drawHeader();
  drawGround();
  drawShip(player_ctr_x, player_ctr_y, player_rotation, false);

  game_state = STATE_PLAYING;
}

/*
 * Draws a Paused message on the screen
 */
void drawPaused()
{
  tft.fillRect(110, 20, 112, 20, ILI9341_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_RED);
  tft.setCursor(110, 20);
  tft.print("** PAUSED **");
  delay(200);
  
  game_state = STATE_PAUSED;
}

/*
 * Erases the Paused message
 */
void erasePaused()
{
  tft.fillRect(110, 20, 112, 20, ILI9341_BLACK);
  delay(200);
  
  game_state = STATE_PLAYING;
}

/*
 * Draws the Game Over screen
 */
void drawGameOver()
{
  drawHeader();
  drawGround();
  tft.fillRect (80, 70, 160, 55, ILI9341_BLACK);  // clear msg area
  tft.drawRect (80, 70, 160, 55, ILI9341_YELLOW); // border
  
  tft.setTextSize(2);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(105, 80);
  tft.print("GAME OVER");

  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(105, 110);
  tft.print("PRESS [Y] TO START");

  game_state = STATE_GAME_OVER;
}

void wipeGameArea()
{
  DacAudio.Play(&warp_play);
  for (int i = 0; i <= 110; i++)
  {
    tft.drawRect(110 - i, 130 - i, i * 2 + 100, i * 2, ILI9341_WHITE);
    DacAudio.FillBuffer(); 
  }
  DacAudio.FillBuffer(); 
  for (int i = 0; i <= 110; i++)
  {
    tft.drawRect(110 - i, 130 - i, i * 2 + 100, i * 2, ILI9341_BLACK);
    DacAudio.FillBuffer(); 
  }
}

/*
 * Plays the player shot sound
 */
void playShot()
{
  DacAudio.Play(&fire_play);
}

/*
 * Draws and plays an explosion sound for a turret or the player ship
 */
void drawExplosion(int x, int y, int maxRadius)
{
  DacAudio.Play(&explosion_play);
  for (int i = 1; i <= maxRadius; i++)
  {
    tft.drawCircle(x, y, i, ILI9341_YELLOW);
    DacAudio.FillBuffer(); 
    delay(10);
  }
  for (int i = 1; i <= maxRadius; i++)
  {
    tft.drawCircle(x, y, i, ILI9341_BLACK);
    DacAudio.FillBuffer(); 
    delay(10);
  }
}

/* 
 *  Plays the fuel pickup sound 
 */
void playFuelSound()
{
  DacAudio.Play(&refuel_play);
}

/*
 * Initializes the player and turret shots to inactive
 */
void initShots()
{
  for (int i = 0; i < MAX_SHOTS; i++)
  {
    if (shot_range[i])  // erase old active shot
      tft.drawRect(shot_x[i], shot_y[i], 2, 2, ILI9341_BLACK);
    
    shot_range[i] = 0;
  }

  for (int i = 0; i < MAX_TURRET; i++)
  {
    if (turret_shot_range[i])  // erase old active turret shot
      tft.drawRect(turret_shot_x[i], turret_shot_y[i], 2, 2, ILI9341_BLACK);
    
    turret_shot_range[i] = 0;
  }
}

/*
 * Initializes game level (ground, fuel packs, turrets) from the level data parameter
 */
void loadLevelData(const int levelData[])
{
  // Load the ground segment data
  for (int i = 0; i < MAX_GROUND; i++)
   {
     if (levelData[i * 2] < 999)
     {
      ground_x[i] = levelData[i * 2];
      ground_y[i] = levelData[i * 2 + 1];
      ground_size = i;
     }
   }
   
   // Load the fuel pack location data
   int fuel_start = MAX_GROUND * 2;
   fuel_count = 0;
   
   for (int i = 0; i < MAX_FUEL; i++)
   {
     if (levelData[fuel_start + i * 2] < 999)
     {
      fuel_x[i] = levelData[fuel_start + i * 2];
      fuel_y[i] = levelData[fuel_start + i * 2 + 1];
      fuel_active[i] = true;
      fuel_count += 1;
     }
     else
     {
       fuel_active[i] = false;
     }
   }
   fuel_remaining = fuel_count;
  
   // Load the enemy turret location data
   int turret_start = fuel_start + MAX_FUEL * 2;
   turret_count = 0;
   
   for (int i = 0; i < MAX_TURRET; i++)
   {
     if (levelData[turret_start + i * 3] < 999)
     {
      turret_x[i] = levelData[turret_start + i * 3];
      turret_y[i] = levelData[turret_start + i * 3 + 1];
      turret_dir[i] = levelData[turret_start + i * 3 + 2];
      turret_active[i] = true;
      turret_count += 1;
     }
     else
     {
       turret_active[i] = false;
     }
   }
   turret_remaining = turret_count;
   
   // Load the miscellanous player/level data
   int misc_start = turret_start + MAX_TURRET * 3;
   
   player_start_x = levelData[misc_start];
   player_start_y = levelData[misc_start + 1];
   level_delay = levelData[misc_start + 2];
   
   startPlayer();
}

/*
 * Initializes game level (ground, fuel packs, turrets) based on the level parameter
 */
void loadLevel(int lvl)
{
  int level_map = lvl;
  while (level_map > MAX_LEVEL)
  {
    level_map -= MAX_LEVEL;
  }
  
  switch(level_map)
  {
  case 1: loadLevelData((int *)Level_01);
    break;
  case 2: loadLevelData((int *)Level_02);
    break;
  case 3: loadLevelData((int *)Level_03);
    break;
  case 4: loadLevelData((int *)Level_04);
    break;
  case 5: loadLevelData((int *)Level_05);
    break;
  case 6: loadLevelData((int *)Level_06);
    break;
  default:  loadLevelData((int *)Level_01);
  }
  
  tft.fillRect(0, 10, 320, 240, ILI9341_BLACK);
}

/*
 * Draws the ground segments for the current level
 */
void drawGround()
{
  for (int i = 0; i < ground_size; i++)
  {
  if (ground_x[i] < INVALID_ITEM && ground_x[i+1] < INVALID_ITEM)
  {
    tft.drawLine(ground_x[i], ground_y[i], ground_x[i+1], ground_y[i+1], ILI9341_GREEN);
  }
  }
}

/*
 * Draws the game header at the top of the screen (fuel, lives, score)
 */
void drawHeader()
{
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_WHITE);
  
  tft.setCursor(0, 1);
  tft.print("FUEL");
  drawPlayerFuel();

  drawPlayerLives();

  tft.setCursor(240, 0);
  tft.print("SCORE: 000000");
  changePlayerScore(0);  
}

/*
 * Draws the player fuel bar
 */
void drawPlayerFuel()
{
  tft.drawRect(PLAYER_FUEL_X - 1, 0, 103, 10, ILI9341_WHITE);
  tft.fillRect(PLAYER_FUEL_X, 1, player_fuel, 8, ILI9341_CYAN);
}

/*
 * Changes the player fuel by the given amount and redraws the fuel bar in the game header
 */
void changePlayerFuel(int changeAmt)
{
  if (changeAmt < 0)
  {
    tft.fillRect(PLAYER_FUEL_X + player_fuel, 1, changeAmt, 8, ILI9341_BLACK); 
    player_fuel += changeAmt;
    return;
  }

  // avoid going over fuel limit
  int chg = changeAmt;
  if (chg + player_fuel > 100)
    chg = 100 - player_fuel;

  tft.fillRect(PLAYER_FUEL_X + player_fuel, 1, chg, 8, ILI9341_CYAN); 
  player_fuel += chg;
  return;
}

/*
 * Changes and displays the player score and awards any free ships
 */
void changePlayerScore(int changeAmt)
{
  player_score += changeAmt;

  tft.fillRect(280,0, 40, 10, ILI9341_BLACK); // erase prev score
  tft.setTextColor(ILI9341_WHITE);
  tft.setCursor(280, 0);
  tft.print(player_score);
  
  if (changeAmt > 0 && player_score >= next_ship)
  {
    player_lives += 1;
    next_ship += 10000;
    drawPlayerLives();
  }
}

/*
 * Draws the player lives remaining in the game header
 */
void drawPlayerLives()
{
    int ctr_x, ctr_y;
    int x[5], y[5];
    
    for (int i = 0; i < MAX_LIVES; i++)
    {
      if (i < player_lives)
      {
        ctr_x = 150 + i * 16;
        ctr_y = 9;
        
        // nose of ship
        x[0] = ctr_x + cos(PIx15) * SHIP_SIZE;
        y[0] = ctr_y + sin(PIx15) * SHIP_SIZE;
        // left wingtip
        x[1] = ctr_x + cos(PIx15 - PId15) * SHIP_SIZE;
        y[1] = ctr_y + sin(PIx15 - PId15) * SHIP_SIZE;
        //center point
        x[2] = ctr_x;
        y[2] = ctr_y;
        // right wingtip
        x[3] = ctr_x + cos(PIx15 + PId15) * SHIP_SIZE;
        y[3] = ctr_y + sin(PIx15 + PId15) * SHIP_SIZE;
        // nose of ship
        x[4] = x[0];
        y[4] = y[0];
  
        // connect ship vertices
        for(int i = 0; i < 4; i++)
        {
          tft.drawLine(x[i], y[i], x[i+1], y[i+1], ILI9341_BLUE);
        }
      }
      else
      {
        // blank out ship space
        tft.fillRect(ctr_x - SHIP_SIZE, 0, 16, 16, ILI9341_BLACK);
      }
    }
}

/*
 * Draws the fuel packs on the game screen
 */
void drawFuelPacks()
{
  for (int i = 0; i < fuel_count; i++)
  {
    if (fuel_active[i])
    {
      drawFuelPack(fuel_x[i], fuel_y[i]);
    }
  }
}

/*
 * Draws a fuel pack at the given x,y coordinate (top left corner)
 */
void drawFuelPack(int x, int y)
{
  tft.drawRect(x, y, FUEL_WD, FUEL_HT, ILI9341_CYAN);
}

/*
 * Erases a fuel pack at the given x,y coordinate (top left corner)
 */
void eraseFuelPack(int x, int y)
{
  tft.drawRect(x, y, FUEL_WD, FUEL_HT, ILI9341_BLACK);
}

/*
 * Draws all turrets and randomly shoots at the player based on the shotRate
 */
void drawTurrets(int shotRate)
{
  int x, y;
  
  for (int i = 0; i < turret_count; i++)
  {
    if (turret_active[i])
    {
      drawTurret(turret_x[i], turret_y[i], turret_dir[i]);
      
      if (shotRate > random(100))
      {
        // try to shoot (from tip of turret)
        x = turret_x[i] + turret_offset_x[turret_dir[i] - 1];
        y = turret_y[i] + turret_offset_y[turret_dir[i] - 1];
        createTurretShot(x, y);
      }
    }
  }
}

/*
 * Draws a turret at the given x,y coordinates
 */
void drawTurret(int x, int y, int direction)
{
  switch(direction)
  {
  case DIR_UP: 
    tft.drawRect(x, y, 20, 10, ILI9341_RED);
    tft.drawRect(x + 5, y - 10, 10, 10, ILI9341_RED);
    break;
  case DIR_DOWN:
    tft.drawRect(x, y, 20, 10, ILI9341_RED);
    tft.drawRect(x + 5, y + 10, 10, 10, ILI9341_RED);
    break;
  case DIR_LEFT:
    tft.drawRect(x, y, 10, 20, ILI9341_RED);
    tft.drawRect(x - 10, y + 5, 10, 10, ILI9341_RED);
    break;
  case DIR_RIGHT:
    tft.drawRect(x, y, 10, 20, ILI9341_RED);
    tft.drawRect(x + 10, y + 5, 10, 10, ILI9341_RED);
    break;
  default:
    tft.drawRect(x, y, 20, 20, ILI9341_RED); // headless
  }
}

/*
 * Erases a turret at the given x,y coordinates
 */
void eraseTurret(int x, int y, int direction)
{
  switch(direction)
  {
  case DIR_UP: 
    tft.drawRect(x, y, 20, 10, ILI9341_BLACK);
    tft.drawRect(x + 5, y - 10, 10, 10, ILI9341_BLACK);
    break;
  case DIR_DOWN:
    tft.drawRect(x, y, 20, 10, ILI9341_BLACK);
    tft.drawRect(x + 5, y + 10, 10, 10, ILI9341_BLACK);
    break;
  case DIR_LEFT:
    tft.drawRect(x, y, 10, 20, ILI9341_BLACK);
    tft.drawRect(x - 10, y + 5, 10, 10, ILI9341_BLACK);
    break;
  case DIR_RIGHT:
    tft.drawRect(x, y, 10, 20, ILI9341_BLACK);
    tft.drawRect(x + 10, y + 5, 10, 10, ILI9341_BLACK);
    break;
  default:
    tft.drawRect(x, y, 20, 20, ILI9341_BLACK); // headless
  }
}

/*
 * Draws the player ship at the given x,y coordinates with the specified rotation and optional thrust flame
 */
void drawShip(int ctrX, int ctrY, double rotation, bool thrust)
{
    // nose of ship
    ship_x[0] = ctrX + cos(rotation) * SHIP_SIZE;
    ship_y[0] = ctrY + sin(rotation) * SHIP_SIZE;
    // left wingtip
    ship_x[1] = ctrX + cos(rotation - PId15) * SHIP_SIZE;
    ship_y[1] = ctrY + sin(rotation - PId15) * SHIP_SIZE;
    //center point
    ship_x[2] = ctrX;
    ship_y[2] = ctrY;
    // right wingtip
    ship_x[3] = ctrX + cos(rotation + PId15) * SHIP_SIZE;
    ship_y[3] = ctrY + sin(rotation + PId15) * SHIP_SIZE;
    // nose of ship
    ship_x[4] = ship_x[0];
    ship_y[4] = ship_y[0];

    // connect ship vertices
    for(int i = 0; i < 4; i++)
    {
      tft.drawLine(ship_x[i], ship_y[i], ship_x[i+1], ship_y[i+1], ILI9341_BLUE);
    }

    if (thrust)
    {
      // left thrust edge
      ship_x[5] = ctrX + cos(rotation - PId15) * THRUST_SIZE;
      ship_y[5] = ctrY + sin(rotation - PId15) * THRUST_SIZE;
      // thrust back tip
      ship_x[6] = ctrX + cos(rotation - PI) * SHIP_SIZE;
      ship_y[6] = ctrY + sin(rotation - PI) * SHIP_SIZE;
      // right thrust edge
      ship_x[7] = ctrX + cos(rotation + PId15) * THRUST_SIZE;
      ship_y[7] = ctrY + sin(rotation + PId15) * THRUST_SIZE;
      // center point
      ship_x[8] = ctrX;
      ship_y[8] = ctrY;

      // connect thrust vertices
      for(int i = 5; i < 8; i++)
      {
        tft.drawLine(ship_x[i], ship_y[i], ship_x[i+1], ship_y[i+1], ILI9341_RED);
      }
    }
    else  // not thrusting, reset exhaust flame
    {
      for(int i = 5; i < 9; i++)
      {
        ship_x[i] = ctrX;
        ship_y[i] = ctrY;
      }
    }    
}

/*
 * Erases the player ship at the given x,y coordinates
 */
void eraseShip()
{
  for(int i = 0; i < 8; i++)
  {
    tft.drawLine(ship_x[i], ship_y[i], ship_x[i+1], ship_y[i+1], ILI9341_BLACK);
  }
}

/*
 * Creates a new player shot from the given x,y coordinates moving in the specified rotation direction
 */
void createPlayerShot(int x, int y, double rotation)
{
  for (int i = 0; i < MAX_SHOTS; i++)
  {
    if (shot_range[i] <= 0)
    {
      shot_x[i] = x;
      shot_y[i] = y;
      shot_range[i] = player_shot_range;
      shot_speed_x[i] = sin(rotation + PId2) * SHOT_SPEED;
      shot_speed_y[i] = -cos(rotation + PId2) * SHOT_SPEED;
      playShot();
      return;
    }
  }
}

/*
 * Moves all the player shots along their current paths
 */
void movePlayerShots()
{
  for (int i = 0; i < MAX_SHOTS; i++)
  {
    if (shot_range[i])
    {
      movePlayerShot(i);
    }
  }
}

/*
 * Moves a specific player shot along its current path and checks for collisions
 */
void movePlayerShot(int idx)
{
  // erase shot
  tft.drawRect(shot_x[idx], shot_y[idx], 2, 2, ILI9341_BLACK);
  
  shot_x[idx] += shot_speed_x[idx];
  shot_y[idx] += shot_speed_y[idx];

  if (shot_x[idx] < 10 || shot_x[idx] > 330 || 
      shot_y[idx] < 20 || shot_y[idx] > 230)
  {
     shot_range[idx] = 0;
     return;
  }

  if (checkTurretHit(shot_x[idx], shot_y[idx], 2) >= 0)
  {
    // hit a turret
    changePlayerScore(500);
    turret_remaining -= 1;
    shot_range[idx] = 0;
    return;
  }

  if (hitsGround(shot_x[idx], shot_y[idx], 5))
  {
    shot_range[idx] = 0;
    return;
  }

  // draw shot
  tft.drawRect(shot_x[idx], shot_y[idx], 2, 2, ILI9341_WHITE);
  shot_range[idx] -= 1;
}

/*
 * Creates a turret shot from the given x,y coordinates towards the player
 */
void createTurretShot(int x, int y)
{
  // introduce some targeting variability
  int rand_x = 16 - random(32);  
  int rand_y = 16 - random(32);

  // compute the angle and speed to the player ship
  double angle = atan2(player_ctr_y - y + rand_y, player_ctr_x - x + rand_x);
  double speed_x = sin(angle + PId2) * (SHOT_SPEED - 2);
  double speed_y = -cos(angle + PId2) * (SHOT_SPEED - 2);

  // find an inactive shot to use
  for (int i = 0; i < MAX_TURRET; i++)
  {
    if (turret_shot_range[i] <= 0)
    {
      turret_shot_x[i] = x;
      turret_shot_y[i] = y;
      turret_shot_range[i] = turret_range;

      turret_shot_speed_x[i] = speed_x;
      turret_shot_speed_y[i] = speed_y;
      playShot();
      return;
    }
  }
}

/*
 * Moves all the turret shots
 * Returns true if a shot hit the player
 */
bool moveTurretShots()
{
  bool hitPlayer;
  for (int i = 0; i < MAX_TURRET; i++)
  {
    if (turret_shot_range[i] > 0)
    {
      hitPlayer = moveTurretShot(i);
      if (hitPlayer)
        return true;
    }
  }
  return false;
}

/*
 * Moves a specific turret shot
 * Returns true if the shot hit the player
 */
bool moveTurretShot(int idx)
{
  // erase shot
  tft.drawRect(turret_shot_x[idx], turret_shot_y[idx], 2, 2, ILI9341_BLACK);
  
  turret_shot_x[idx] += turret_shot_speed_x[idx];
  turret_shot_y[idx] += turret_shot_speed_y[idx];

  if (turret_shot_x[idx] < 10 || turret_shot_x[idx] > 330 || 
      turret_shot_y[idx] < 20 || turret_shot_y[idx] > 230)
  {
     turret_shot_range[idx] = 0;
     return false;
  }

  if (hitsPlayer(turret_shot_x[idx], turret_shot_y[idx], SHIP_SIZE))
  {
    turret_shot_range[idx] = 0;
    killPlayer();
    return true;
  }

  if (hitsGround(turret_shot_x[idx], turret_shot_y[idx], 5))
  {
     turret_shot_range[idx] = 0;
     return false;
  }

  turret_shot_range[idx] -= 1;
  if (turret_shot_range[idx] > 0)
    tft.drawRect(turret_shot_x[idx], turret_shot_y[idx], 2, 2, ILI9341_RED);
  return false;
}

/*
 * Returns true if the given x,y coordinate hits the player ship within the specified tolerance
 */
bool hitsPlayer(int x, int y, double tolerance)
{
  return (abs(x - player_ctr_x) < tolerance && abs(y - player_ctr_y) < tolerance);

}

/*
 * Returns the a fuel index if the given x,y coordinate hits any fuel packs within the specified tolerance
 * Returns -1 if no fuel is at that coordinate
 */
int checkFuelHit(int x, int y, double tolerance)
{
  for (int i = 0; i < fuel_count; i++)
  {
    if (fuel_active[i])
    {
      if (hitsFuel(i, x, y, tolerance))
      {
        playFuelSound();
        eraseFuelPack(fuel_x[i], fuel_y[i]);
        fuel_active[i] = false;
        return i;
      }
    }
  }
  return -1;  // nothing hit
}

/*
 * Returns true if the given x,y coordinate hits a particular fuel pack within the specified tolerance
 */
bool hitsFuel(int idx, int x, int y, double tolerance)
{
  if (x >= fuel_x[idx] - tolerance && x <= fuel_x[idx] + FUEL_WD + tolerance &&  
      y >= fuel_y[idx] - tolerance && y <= fuel_y[idx] + FUEL_HT + tolerance)
  {
    return true; 
  }

  return false;
}

/*
 * Returns the turret index if the given coordinate hits any turret within the specified tolerance
 * Returns -1 if no turret is at that coordinate
 */
int checkTurretHit(int x, int y, double tolerance)
{
  for (int i = 0; i < turret_count; i++)
  {
    if (turret_active[i])
    {
      if (hitsTurret(i, x, y, tolerance))
      {
        drawExplosion(turret_x[i] + 10, turret_y[i], 12);
        eraseTurret(turret_x[i], turret_y[i], turret_dir[i]);
        turret_active[i] = false;
        return i;
      }
    }
  }
  return -1;  // nothing hit
}

/*
 * Returns true if the given x,y coordinate hits a particular turret within the specified tolerance
 */
bool hitsTurret(int idx, int x, int y, double tolerance)
{
  switch(turret_dir[idx])
  {
    case DIR_UP: 
      //tft.drawRect(x, y, 20, 10, ILI9341_RED);
      //tft.drawRect(x + 5, y - 10, 10, 10, ILI9341_RED);
      if (x >= turret_x[idx] && x <= turret_x[idx] + 20 &&  
        y >= turret_y[idx] && y <= turret_y[idx] + 10)
        return true;
      if (x >= turret_x[idx] +  5 && x <= turret_x[idx] + 15 &&  
        y >= turret_y[idx] - 10 && y <= turret_y[idx])
        return true;
      break;
    case DIR_DOWN:
      //tft.drawRect(x, y, 20, 10, ILI9341_RED);
      //tft.drawRect(x + 5, y + 10, 10, 10, ILI9341_RED);
      if (x >= turret_x[idx] && x <= turret_x[idx] + 20 &&  
        y >= turret_y[idx] && y <= turret_y[idx] + 10)
        return true;
      if (x >= turret_x[idx] +  5 && x <= turret_x[idx] + 15 &&  
        y >= turret_y[idx] && y <= turret_y[idx] + 10)
        return true;
      break;
    case DIR_LEFT:
      //tft.drawRect(x, y, 10, 20, ILI9341_RED);
      //tft.drawRect(x - 10, y + 5, 10, 10, ILI9341_RED);
      if (x >= turret_x[idx] && x <= turret_x[idx] + 10 &&  
        y >= turret_y[idx] && y <= turret_y[idx] + 20)
        return true;
      if (x >= turret_x[idx] - 10 && x <= turret_x[idx] &&  
        y >= turret_y[idx] + 5 && y <= turret_y[idx] + 15)
        return true;
      break;
    case DIR_RIGHT:
      //tft.drawRect(x, y, 10, 20, ILI9341_RED);
      //tft.drawRect(x + 10, y + 5, 10, 10, ILI9341_RED);
      if (x >= turret_x[idx] && x <= turret_x[idx] + 10 &&  
        y >= turret_y[idx] && y <= turret_y[idx] + 20)
        return true;
      if (x >= turret_x[idx] + 10 && x <= turret_x[idx] + 20 &&  
        y >= turret_y[idx] + 5 && y <= turret_y[idx] + 15)
        return true;
      break;
    default:
      return false;
  }

  return false;
}

/*
 * Returns true if the given x,y coordinate hits any ground segment within the specified tolerance
 */
bool hitsGround(int x, int y, double tolerance)
{
  for (int i = 0; i < ground_size; i++)
  {
  if (ground_x[i] < INVALID_ITEM && ground_x[i+1] < INVALID_ITEM)
  {
    if (hitsLine(x, y, ground_x[i], ground_y[i], ground_x[i+1], ground_y[i+1], tolerance))
    {
    // debug option to flash ground segment when hit is detected
      //tft.drawLine(ground_x[i], ground_y[i], ground_x[i+1], ground_y[i+1], ILI9341_WHITE);
      //delay(200);
      //tft.drawLine(ground_x[i], ground_y[i], ground_x[i+1], ground_y[i+1], ILI9341_GREEN);
      return true;
    }
  }
  }
  return false;
}

/*
 * Returns true if the give x,y coordinate is on the line with endpoints X1,Y1 and X2,Y2 within the specified tolerance
 */
bool hitsLine(int x, int y, int lineX1, int lineY1, int lineX2, int lineY2, double tolerance)
{
  bool result = false;

  // quick bounding box check
  if (x < min(lineX1, lineX2) - tolerance || 
      x > max(lineX1, lineX2) + tolerance ||
      y < min(lineY1, lineY2) - tolerance || 
      y > max(lineY1, lineY2) + tolerance)
  {
    return false;       
  }
  
  // do actual linear equation check: y = mx + b
  // TODO: precompute slope (m) and x-intercept (b) for speed
  int  m_rise = lineY2 - lineY1; 
  int  m_run  = lineX2 - lineX1;
  double m_slope = 0;
  double b = 0;
  
  if (m_run > 0)
  {
    m_slope = m_rise / m_run;
    b = lineY1 - (m_slope * lineX1);
    if (abs(y - (m_slope * x) - b) < tolerance)
      result = true; 
  }
  else // we have a vertical line here
  {
    if (x == lineX1 && y <= max(lineY1, lineY2) && y >= min(lineY1, lineY2))
      result = true;
  }

  return result;
}

/*
 * Initialize the player ship
 */
void startPlayer()
{
  initShots();
  player_ctr_x = player_start_x; 
  player_ctr_y = player_start_y;
  player_speed_x = 0;
  player_speed_y = 0;
  player_rotation = PIx15; // up
  player_fuel = 100;
  delay(200);
  drawPlayerFuel();
  drawPlayerLives();
}

/*
 * Destroys the player's ship
 */
void killPlayer()
{ 
  eraseShip();
  drawExplosion(player_ctr_x, player_ctr_y, 10);
  
  player_lives -= 1;
  if (player_lives < 0)
  {
    drawGameOver();
    return;
  }

  drawHeader();
  startPlayer();
  drawShip(player_ctr_x, player_ctr_y, player_rotation, false);
}

/*
 * Moves player to the next level in the game
 */
void advanceLevel()
{
  changePlayerScore(1000); // level bonus

  for (int i = player_fuel; i >= 0; i--)
  {
    // play tick sound
    DacAudio.Play(&tick_play);
    changePlayerScore(10);
    DacAudio.FillBuffer(); 
    changePlayerFuel(-1);
    DacAudio.FillBuffer(); 
  }
  
  level += 1;
  turret_range += 1;
  loadLevel(level);
  wipeGameArea();
  drawGround();
}

/*
 * Main program loop
 */
void loop() 
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

  DacAudio.FillBuffer(); // load data for any pending audio samples
  btnB_pressed_prev = btnB_pressed;  // used to prevent double firing
  delay(level_delay);
}

/*
 * Handles the STATE_TITLE game state logic 
 */
void handleTitle()
{
  if (btnX_pressed)
  {
    drawRulesScreen();
    return;
  }

  if (btnY_pressed)
  {
    startGame(1);
    return;
  }

  // secret start level hack
  if (btnA_pressed)
  {
    //left = bit 0, up = bit 1, down = bit 2, righ = bit 3
    int startLevel = (btnLeft_pressed ? 1 : 0) + (btnUp_pressed ? 2 : 0) + (btnDown_pressed ? 4 : 0) + (btnRight_pressed ? 8 : 0); 
    startGame(startLevel);
  }
}

/*
 * Handles the STATE_RULES game state logic
 */
void handleRules()
{
  if (btnY_pressed)
  {
    startGame(1);
  }
}

/*
 * Handles the STATE_PLAYING game state logic
 */
void handlePlaying()
{
  // player controls logic
  if (btnX_pressed)
  {
    drawPaused();
    return;
  }
  
  if (btnLeft_pressed)
  {
    player_rotation -= 0.1;
  }

  if (btnRight_pressed)
  {
    player_rotation += 0.1;
  }
  
  player_speed_y += gravity;

  if (btnA_pressed)
  {
    if (player_fuel > 0)
    {
      player_speed_x += sin(player_rotation + PId2) / 8;
      player_speed_y -= cos(player_rotation + PId2) / 8;
      changePlayerFuel(-1);
    }
  }

  player_ctr_x += player_speed_x;
  player_ctr_y += player_speed_y;

  // screen bounds checking
  if ((player_ctr_x > SCREEN_WD - SHIP_SIZE && player_speed_x > 0) || 
      (player_ctr_x < SHIP_SIZE && player_speed_x < 0))
  {
    player_speed_x = -player_speed_x;
    player_ctr_x += player_speed_x;
  }

  if (((player_ctr_y > (SCREEN_HT - SHIP_SIZE)) && (player_speed_y > 0)) || 
      (player_ctr_y <  20 && (player_speed_y < 0)))
  {
    player_speed_y = -player_speed_y;
    player_ctr_y += player_speed_y;
  }

  if (btnB_pressed && !btnB_pressed_prev)
  {
    createPlayerShot(player_ctr_x, player_ctr_y, player_rotation);
  }

  // game object hit detection and updates/animation
  drawGround();
  drawFuelPacks();
  drawTurrets(2);
  moveTurretShots();
  if (game_state != STATE_PLAYING)  // in case player died
    return;
    
  if (checkFuelHit(player_ctr_x, player_ctr_y, SHIP_SIZE) >= 0)
  {
    changePlayerScore(200);
    changePlayerFuel(50); 
    fuel_remaining -= 1;
  }

  // check the ship vertices for ground collision
  if (hitsGround(player_ctr_x, player_ctr_y, SHIP_SIZE - 2))
  {
    killPlayer();
    return;
  }

  // check the ship vertices for ground collision
  // this is more accurate, though slower - especially with more ground segments
  /*
  for(int i = 0; i < 4; i++)
  {
    if (hitsGround(ship_x[i], ship_y[i], 1))
    {
      killPlayer();
      return;
    }
  }
  */
  
  
  eraseShip();
  drawShip(player_ctr_x, player_ctr_y, player_rotation, btnA_pressed && player_fuel > 0);

  movePlayerShots();

  if (turret_remaining == 0 && fuel_remaining == 0)
  {
    advanceLevel();
  }
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
