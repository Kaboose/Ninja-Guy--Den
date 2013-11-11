#include "SDL.h"
#include "SDL_image.h"
#include "Timer.h"
#include "Tile.h"
#include <string>
#include <sstream>
#include <vector>

using namespace std;

typedef enum { SMALL_ORC } enemies_type;
int num_small_orcs = 0;

//Used for determing where collisions are
typedef enum { LEFT = 0, RIGHT, BOTTOM } sides;

//Pretty self explanatory variables
const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SCREEN_BPP = 32;
const int FPS = 60;
int frame, start_x, start_y;

//Camera
SDL_Rect Camera = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };

SDL_Color textColor = {255,255,255};

//Read the LazyFoo tutorials to better understand
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL; //<-Use to render
SDL_Texture* screen; //<-Render to

SDL_Event event;

Timer fps;

//Current Level
Tile** CurrentLevel;
int LevelSize;
int LevelWidth;
int LevelHeight;

bool checkCollision( SDL_Rect a, SDL_Rect b )
{
    //The sides of the rectangles
    int leftA, leftB;
    int rightA, rightB;
    int topA, topB;
    int bottomA, bottomB;

    //Calculate the sides of rect A
    leftA = a.x;
    rightA = a.x + a.w;
    topA = a.y;
    bottomA = a.y + a.h;

    //Calculate the sides of rect B
    leftB = b.x;
    rightB = b.x + b.w;
    topB = b.y;
    bottomB = b.y + b.h;

    //If any of the sides from A are outside of B
    if( bottomA <= topB )
    {
        return false;
    }

    if( topA >= bottomB )
    {
        return false;
    }

    if( rightA <= leftB )
    {
        return false;
    }

    if( leftA >= rightB )
    {
        return false;
    }

    //If none of the sides from A are outside B
    return true;
}

//Moveable Class and Functions
#pragma region Moveable
class Moveable
{
public:
	int X_VELOCITY;
	int Y_VELOCITY;
	int MAX_Y_VELOCITY;

	int xvel;
	float yvel;

	SDL_Rect box;

	SDL_Rect getBox(); //Gets the character's current location
	void setXVel(int x);
	void setYVel(int y);
	int getXVel();
	int getYVel();

	SDL_Rect ground_tile;
};

SDL_Rect Moveable::getBox()
{
	return box;
}

void Moveable::setXVel(int x)
{
	xvel = x;
}

void Moveable::setYVel(int y)
{
	yvel = y;
}

int Moveable::getXVel()
{
	return xvel;
}

int Moveable::getYVel()
{
	return yvel;
}
#pragma endregion Moveable Class


//Weapon Class and Functions
#pragma region Weapon
class Weapon
{
public:
	void init(SDL_Texture* texture, SDL_Rect box, float xvel, float yvel);
	void update();
	void draw();
	SDL_Rect getBox();

	SDL_Texture* texture;
	SDL_Rect box;
	int xvel, yvel;
	int angle;
};

void Weapon::init(SDL_Texture* texture, SDL_Rect box, float xvel, float yvel)
{
	this->texture = texture;
	this->box = box;
	this->xvel = xvel;
	this->yvel = yvel;
	angle = 0;
}

void Weapon::update()
{
	box.x += xvel;
	box.y += yvel;
	angle += 50;
}

void Weapon::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	SDL_RenderCopyEx(renderer, texture, NULL, &destination, angle, NULL, flip);
}

SDL_Rect Weapon::getBox()
{
	return box;
}
#pragma endregion Weapon Class


//Character Class and Functions
#pragma region Character
class Character : public Moveable
{
private:
	int FACE_RIGHT; //Sprite index
	int FACE_LEFT;
	int STAND_LEFT;
	int STAND_RIGHT;
	int WALKING_LENGTH;

	SDL_Rect clip[32]; //An array that stores every clipped sprite from the sprite sheet
	int currentClip; //Index of the current clip in clip[]

	int direction;

	//Textures
	SDL_Texture *texture;
	SDL_Texture *ninja_star_texture;
	SDL_Texture* health_bar;

	int beginFrame; //Used to know when buttons were pressed to regulate animation properly

	//These are for preventing conflicts: walking and jumping, walking two directions, jumping twice, etc
	bool jumping;
	bool falling;
	bool jumpable;
	bool walkLeft;
	bool walkRight;
	bool walkable;
	bool stop;
	bool running;
	bool throwing;

	//Weapon index
	Weapon ninja_star;

	//Health
	int health;
	SDL_Rect health_box;

public:
	Character(); //Constructor
	void init(int x, int y); //Initializes at a specific x,y location
	void load(); //Loads character texture
	void move(); //Moves the character
	void handleInput(SDL_Event event); //Handles the input
	void show(int x, int y); //Draws the character on the screen

	void checkCollision();
	bool collision[3];

	//Health
	void drawHealth();
	void damage(int amount);

	SDL_Texture* loadImage(const char *filename);
};

Character::Character()
{
	texture = NULL;
	health_bar = NULL;
	int width = 50; //Width and height of each clipped sprite from the sprite sheet
	int height = 77;

	box.w = 40;
	box.h = 40;

	//Arbitrary values
	X_VELOCITY = 4;
	Y_VELOCITY = 1;
	MAX_Y_VELOCITY = 20;

	FACE_LEFT = 24;
	FACE_RIGHT = 28;
	STAND_LEFT = 0;
	STAND_RIGHT = 8;
	WALKING_LENGTH = 7;

	currentClip = 8;
	clip[0].x = 0;
	clip[0].y = 77;
	clip[0].w = width;
	clip[0].h = height;

	//Runs through the sprite sheet and loads in each clip
	for (int i = 1; i < 32; i++)
	{
		clip[i].x = clip[i-1].x + width;
		clip[i].y = clip[i-1].y;
		if (i%8 == 0)
		{
			clip[i].x = 0;
			clip[i].y += height;
		}
		clip[i].w = width;
		clip[i].h = height;
	}

	beginFrame = 0;
	jumping = false;
	falling = false;
	jumpable = true;
	walkLeft= false;
	walkRight = false;
	walkable = true;
	stop = true;
	running = false;
	throwing = false;

	//Health
	health = 100;
	SDL_Rect temp = { 10, 40, health*3, 10 };
	health_box = temp;
}

void Character::init(int x, int y)
{
	xvel = 0;
	yvel = 0;

	box.x = x;
	box.y = y;

	direction = FACE_RIGHT;

	ground_tile.y = box.y + box.h;
	ground_tile.x = box.x;
	ground_tile.w = box.w;
	ground_tile.h = box.h;
}

void Character::load()
{
	texture = loadImage("Media/character.png");
	ninja_star_texture = loadImage("Media/Objects/ninjastar.png");
	health_bar = loadImage("Media/Health.png");
}

SDL_Texture* Character::loadImage(const char *filename)
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( filename );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError() );
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	return newTexture;
}

void Character::handleInput(SDL_Event event)
{
	if( event.type == SDL_KEYDOWN )
    {
        switch( event.key.keysym.sym )
        {
		case SDLK_a:
			if (event.key.repeat == 0 && walkable)
			{
				direction = FACE_LEFT;
				walkRight = false;
				beginFrame = frame;
				walkLeft = true;
				currentClip = 1;
			}
			break;
		case SDLK_d:
			if (event.key.repeat == 0 && walkable)
			{
				direction = FACE_RIGHT;
				walkLeft= false;
				beginFrame = frame;
				walkRight = true;
				currentClip = 9;
			}
			break;
		case SDLK_w:
			if (jumpable)
			{
				currentClip = direction;
				walkable = false;
				jumping = true;
				jumpable = false;
				yvel = -MAX_Y_VELOCITY;
				beginFrame = frame;
			}
			break;
		case SDLK_RSHIFT:
			running = true;
        }
	}
	else if( event.type == SDL_KEYUP )
    {
        switch( event.key.keysym.sym )
        {
            case SDLK_a:
				if (walkLeft && !walkRight) 
				{
					if (!jumping)
						currentClip = 0;
					walkLeft = false;
				}
				break;
            case SDLK_d: 
				if (walkRight && !walkLeft) 
				{
					if (!jumping)
						currentClip = 8;
					walkRight = false;
				}
				break;
			case SDLK_RSHIFT:
				running = false;
        }        
    }

	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			int target_x, target_y;
			SDL_GetMouseState(&target_x, &target_y);
			target_x += Camera.x;
			target_y += Camera.y;
			SDL_Rect temp = {box.x, box.y+10, 20, 20};
			int delta_x = target_x-box.x;
			int delta_y = target_y-box.y;
			float scale = delta_x*delta_x + delta_y*delta_y;
			scale = sqrt(scale);
			scale = 20/scale;

			if (!throwing)
			{
				throwing = true;
				ninja_star.init(ninja_star_texture, temp, xvel + delta_x*scale, delta_y*scale);
			}
		}
	}
}

void Character::move()
{
	if (collision[RIGHT] || collision[LEFT])
			xvel = 0;

	if (jumping)
	{
		yvel += Y_VELOCITY;
		if (yvel > MAX_Y_VELOCITY)
			yvel = MAX_Y_VELOCITY;

		if (frame - beginFrame < 2)
			currentClip++;
		if (yvel == 0)
		{
			jumping = false;
			falling = true;
			yvel = -yvel;
		}
	}
	else if (falling)
	{
		jumpable = false;

		yvel += Y_VELOCITY;
		if (collision[BOTTOM])
		{
			if (((box.x >= ground_tile.x && box.x <= ground_tile.x + ground_tile.w) ||
				(box.x <= ground_tile.x && (box.x + box.w) >= ground_tile.x)) &&
				box.y >= ground_tile.y - ground_tile.h)
			{
				box.y = ground_tile.y - box.h;
				jumping = false;
				jumpable = true;
				walkable = true;
				falling = false;
				yvel = 0;
			}
		}
	}
	else if (walkLeft)
	{
		if (!collision[LEFT])
		{
			xvel = -X_VELOCITY;
			if (running)
				xvel = -2*X_VELOCITY;
		}
		else
			xvel = 0;
		if ((frame - beginFrame)%4 == 0)
			currentClip++; //Animate to next clip every 8 frames
		if (currentClip > 7) //If outside the current direction walking index, loop
			currentClip = 1;
	}
	else if (walkRight)
	{
		if (!collision[RIGHT])
		{
			xvel = X_VELOCITY;
			if (running)
				xvel = 2*X_VELOCITY;
		}
		else
			xvel = 0;
		if ((frame - beginFrame)%4 == 0)
			currentClip++;
		if (currentClip > 15)
			currentClip = 9;
	}
	else
	{
		xvel = 0; //Stops motion
		if (direction == FACE_RIGHT) //Resets to standing sprites (based on direction)
			currentClip = 8;
		else
			currentClip = 0;
	}

	if (ground_tile.y - box.y > box.h)
		falling = true;

	box.x += xvel;
	box.y += yvel;
	
	if (box.x < 0 || box.x > LevelWidth - box.w) //Binds character to screen
		box.x -= xvel;

	//Health
	health_box.w = health*3;

	//Ninja star
	if (ninja_star.texture != NULL)
		ninja_star.update();

	if (ninja_star.box.x > Camera.x + Camera.w ||
		ninja_star.box.x < Camera.x - ninja_star.box.w ||
		ninja_star.box.y < Camera.y - ninja_star.box.h ||
		ninja_star.box.y > Camera.y + Camera.h)
	{
		ninja_star.texture = NULL;
		throwing = false;
	}
}

void Character::show(int x, int y)
{
	if (ninja_star.texture != NULL)
		ninja_star.draw();

	SDL_Rect destination = { box.x - x, box.y - y, box.w, box.h };

	//LazyFoo** must pass in the renderer, texture, reference to the sprite (as a rectangle), and reference to the destination on the screen
	SDL_RenderCopy(renderer, texture, &clip[currentClip], &destination);
}

void Character::checkCollision()
{
	int index = (box.y/40)*LevelWidth/40 + (box.x/40);
	if (index >= LevelSize)
	{
		collision[LEFT] = false;
		collision[RIGHT] = false;
	}
	else 
	{
		if (CurrentLevel[index] == NULL)
			collision[LEFT] = false;
		else
			collision[LEFT] = CurrentLevel[index]->collidable();
		
		if (CurrentLevel[index+1] == NULL)
			collision[RIGHT] = false;
		else
			collision[RIGHT] = CurrentLevel[index+1]->collidable();

		collision[BOTTOM] = false;
		if (yvel < 0)
			collision[BOTTOM] = false;
		else
		{
			int row = 1;
			while (index+LevelWidth/40*row+(xvel*row)/40 < LevelSize && collision[BOTTOM] == false)
			{
				if (CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40] != NULL &&
					CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40]->collidable())
				{
					ground_tile = CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40]->getBox();
					collision[BOTTOM] = true;
				}
				row++;
			}
		}
	}
}

void Character::drawHealth()
{
	SDL_RenderCopy(renderer, health_bar, NULL, &health_box);
}

void Character::damage(int amount)
{
	health -= amount;
}
#pragma endregion Character Class

//Character
Character Player;


//SmallOrc Class and Functions
#pragma region SmallOrc
class SmallOrc : public Moveable
{
public:
	typedef enum { STANDING_LEFT, STANDING_RIGHT, RUNNING_LEFT, RUNNING_RIGHT, RECOVER } states;
	typedef enum { LEFT = 0, RIGHT, BOTTOM } sides;
	SmallOrc(SDL_Texture* texture, int x, int y);
	void update();
	void draw();

	void setState(states state);
	int getState();

	bool collision[4];

private:
	states state;
	SDL_RendererFlip flip;

	SDL_Texture* texture;
	SDL_Rect clip[12];
	int currentClip;
	int index;
	int beginFrame;
};

SmallOrc::SmallOrc(SDL_Texture *texture, int x, int y)
{
	state = STANDING_LEFT;
	X_VELOCITY = 4;

	index = 1;

	int idle_width = 48, idle_height = 72;
	int run_width = 64, run_height = 72;

	for (int i = 0; i < 12; i++)
	{
		if (i < 4)
		{
			clip[i].x = i%4*idle_width;
			clip[i].y = 0;
			clip[i].w = idle_width;
			clip[i].h = idle_height;
			continue;
		}
		clip[i].x = i%4*run_width;
		clip[i].y = i/4*run_height;
		clip[i].w = run_width;
		clip[i].h = run_height;
	}

	box.w = idle_width;
	box.h = idle_height;

	this->texture = texture;
	box.x = x - (box.w - 40);
	box.y = y - (box.h - 45);

	currentClip = 0;

	xvel = 0;
	yvel = 0;

	collision[LEFT] = false;
	collision[RIGHT] = false;
}

void SmallOrc::update()
{
	
	if (state == STANDING_LEFT || state == STANDING_RIGHT || state == RECOVER)
	{
		if ((frame-beginFrame) % 6 == 0)
			currentClip += index;

		if (currentClip > 3 || currentClip < 0)
		{
			index *= -1;
			currentClip += index;
		}

		xvel = 0;

		if (state == RECOVER && (frame-beginFrame+1)%75 == 0)
			state = STANDING_LEFT;
	}

	if (state == RUNNING_LEFT || state == RUNNING_RIGHT)
	{
		if ((frame-beginFrame)%4 == 0)
			currentClip++;
		
		if (currentClip > 11)
			currentClip = 4;

		if (state == RUNNING_LEFT)
		{
			xvel = -X_VELOCITY;
		}
		else
		{
			xvel = X_VELOCITY;
		}

		if (frame-beginFrame > 60)
		{
			if (state == RUNNING_LEFT)
				setState(STANDING_LEFT);
			else
				setState(STANDING_RIGHT);
		}

		if (collision[LEFT] && state == RUNNING_LEFT)
		{
			xvel = 0;
			setState(STANDING_LEFT);
		}

		if (collision[RIGHT] && state == RUNNING_RIGHT)
		{
			xvel = 0;
			setState(STANDING_RIGHT);
		}
	}

	if (state == STANDING_LEFT || state == RUNNING_LEFT)
		flip = SDL_FLIP_HORIZONTAL;
	else
		flip = SDL_FLIP_NONE;

	box.w = clip[currentClip].w;
	box.h = clip[currentClip].h;

	box.x += xvel;
}

void SmallOrc::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RenderCopyEx(renderer, texture, &clip[currentClip], &destination, 0, NULL, flip);
}

void SmallOrc::setState(states state)
{
	this->state = state;
	beginFrame = frame;
	if (state == STANDING_LEFT || state == STANDING_RIGHT || state == RECOVER)
		currentClip = 0;
	else
		currentClip = 4;
}

int SmallOrc::getState()
{
	return state;
}
#pragma endregion Small Orc Class


//EnemyManager Class and Functions
#pragma region EnemyManager
class EnemyManager
{
private:
	SDL_Texture* small_orc_texture;
	SDL_Texture* loadImage(const char *filename);
	std::vector<SmallOrc> small_orcs;
	bool cameraCollision(SDL_Rect box);

public:
	void addSmallOrc(int x, int y);
	void load();
	void update();
	void draw();
};

SDL_Texture* EnemyManager::loadImage(const char *filename)
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( filename );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError() );
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	return newTexture;
}

void EnemyManager::load()
{
	small_orc_texture = loadImage("Media/Enemies/Small Orc.png");
}

void EnemyManager::addSmallOrc(int x, int y)
{
	small_orcs.push_back(SmallOrc(small_orc_texture, x, y));
}

void EnemyManager::update()
{
	for (int i = 0; i < small_orcs.size(); i++)
	{
		SDL_Rect player = Player.getBox();
		SDL_Rect enemy = small_orcs.at(i).getBox();
		SmallOrc *orc = &small_orcs.at(i);

		//Move orc
		if (((player.y + player.h < enemy.y + enemy.h && player.y + player.h > enemy.y) ||
			(player.y < enemy.y + enemy.h && player.y > enemy.y)) &&
			(orc->getState() == orc->STANDING_LEFT || orc->getState() == orc->STANDING_RIGHT))
		{
			int delta_x = player.x - enemy.x;

			if (delta_x < 200 && delta_x > 0 && orc->getState() != orc->RECOVER)
				orc->setState(orc->RUNNING_RIGHT);
			else if (delta_x > -200 && delta_x < 0 && orc->getState() != orc->RECOVER)
				orc->setState(orc->RUNNING_LEFT);
		}

		//Check collision for orc
		int index = (enemy.y/40)*LevelWidth/40 + (enemy.x/40);
		if (index >= LevelSize)
		{
			orc->collision[orc->LEFT] = false;
			orc->collision[orc->RIGHT] = false;
		}
		else 
		{
			if (CurrentLevel[index] == NULL)
				orc->collision[orc->LEFT] = false;
			else
				orc->collision[orc->LEFT] = CurrentLevel[index]->collidable();

			if (CurrentLevel[index+1] == NULL)
				orc->collision[orc->RIGHT] = false;
			else
				orc->collision[orc->RIGHT] = CurrentLevel[index+1]->collidable();
		}

		//Damage
		if (checkCollision(player, enemy))
		{
			Player.damage(5);
			if (Player.getYVel() > 0)
			{
				Player.setYVel(-1*Player.getYVel());
			}
			if (Player.getYVel() == 0)
			{
				Player.setYVel(-10);
			}
			Player.setXVel(enemy.x < player.x ? 4 : -4);

			orc->setState(orc->RECOVER);
			
		}

		//Update
		if (cameraCollision(orc->getBox()))
			orc->update();
	}
}

void EnemyManager::draw()
{
	for (int i = 0; i < small_orcs.size(); i++)
	{
		if (cameraCollision(small_orcs.at(i).getBox()))
			small_orcs.at(i).draw();
	}
}

bool EnemyManager::cameraCollision(SDL_Rect box)
{
	int topCam = Camera.y;
	int botCam = Camera.y + Camera.h;
	int leftCam = Camera.x;
	int rightCam = Camera.x + Camera.w;

	int top = box.y;
	int bot = box.y + box.h;
	int left = box.x;
	int right = box.x + box.w;

	if (right < leftCam || left > rightCam || top > botCam || bot < topCam)
		return false;

	return true;
}
#pragma endregion EnemyManager Class

//Enemy Manager
EnemyManager EnemyMang;


//Level Manager Class and Functions
#pragma region LevelManager
class LevelManager
{
public:
	typedef enum { HOME_LEVEL } level_type;
	void loadTextures();
	SDL_Texture* loadImage(const char *filename);
	void initializeLevels();
	void loadLevel(level_type level);
	void draw();

	int getWidth();
	int getHeight();
	int getSize();

	SDL_Rect getBoxes(int i);
	Tile** getLevel();

private:
	//Global
	int texel_height;
	int texel_width;

	//Level index
	int current_level;
	int current_height;
	int current_width;

	typedef enum { NE1, NW1, SE1, SW1, NE2, NW2, SE2, SW2, NE3, NW3, SE3, SW3, NE4, NW4, SE4, SW4, NE5, NW5, SE5, SW5, //Walls
		WE1, WW1, WE2, WW2, //Walkways
		TN1, TS1, TN2, TS2, //Totems
		CES, FBG, FLA, GRA, NBG, ROT, //Cat's Eyes, Far Background, Flare, Grass, Nearbackground, Roots
		NON, //No texture

		MCH, //Main Character
		SOR //Small Orc
	} textures;

	//Home Level
	int home_texelw;
	int home_texelh;
	int home_level_width;
	int home_level_height;
	int home_level_size;
	SDL_Texture *home_level_background[2];
	SDL_Texture *home_level_textures[780];
	SDL_Rect *background;
	Tile *home_level[780];

#pragma region Textures
	//Textures
	SDL_Texture *wall_ne1;
	SDL_Texture *wall_nw1;
	SDL_Texture *wall_se1;
	SDL_Texture *wall_sw1;
	SDL_Texture *wall_ne2;
	SDL_Texture *wall_nw2;
	SDL_Texture *wall_se2;
	SDL_Texture *wall_sw2;
	SDL_Texture *wall_ne3;
	SDL_Texture *wall_nw3;
	SDL_Texture *wall_se3;
	SDL_Texture *wall_sw3;
	SDL_Texture *wall_ne4;
	SDL_Texture *wall_nw4;
	SDL_Texture *wall_se4;
	SDL_Texture *wall_sw4;
	SDL_Texture *wall_ne5;
	SDL_Texture *wall_nw5;
	SDL_Texture *wall_se5;
	SDL_Texture *wall_sw5;

	SDL_Texture *walk_w1;
	SDL_Texture *walk_e1;
	SDL_Texture *walk_w2;
	SDL_Texture *walk_e2;

	SDL_Texture *totem_n1;
	SDL_Texture *totem_s1;
	SDL_Texture *totem_n2;
	SDL_Texture *totem_s2;

	SDL_Texture *cat;
	SDL_Texture *far_background;
	SDL_Texture *flare;
	SDL_Texture *grass;
	SDL_Texture *near_background;
	SDL_Texture *roots;
#pragma endregion Texture Declarations

	void buildLevel(textures blueprint[], Tile *level_boxes[], int height, int width, int tex_width, int tex_height);
};

void LevelManager::loadTextures()
{
#pragma region Textures
	wall_ne1 = loadImage("Media/Walls/Wall 1 NE.png");
	wall_nw1 = loadImage("Media/Walls/Wall 1 NW.png");
	wall_se1 = loadImage("Media/Walls/Wall 1 SE.png");
	wall_sw1 = loadImage("Media/Walls/Wall 1 SW.png");
	wall_ne2 = loadImage("Media/Walls/Wall 2 NE.png");
	wall_nw2 = loadImage("Media/Walls/Wall 2 NW.png");
	wall_se2 = loadImage("Media/Walls/Wall 2 SE.png");
	wall_sw2 = loadImage("Media/Walls/Wall 2 SW.png");
	wall_ne3 = loadImage("Media/Walls/Wall 3 NE.png");
	wall_nw3 = loadImage("Media/Walls/Wall 3 NW.png");
	wall_se3 = loadImage("Media/Walls/Wall 3 SE.png");
	wall_sw3 = loadImage("Media/Walls/Wall 3 SW.png");
	wall_ne4 = loadImage("Media/Walls/Wall 4 NE.png");
	wall_nw4 = loadImage("Media/Walls/Wall 4 NW.png");
	wall_se4 = loadImage("Media/Walls/Wall 4 SE.png");
	wall_sw4 = loadImage("Media/Walls/Wall 4 SW.png");
	wall_ne5 = loadImage("Media/Walls/Wall 5 NE.png");
	wall_nw5 = loadImage("Media/Walls/Wall 5 NW.png");
	wall_se5 = loadImage("Media/Walls/Wall 5 SE.png");
	wall_sw5 = loadImage("Media/Walls/Wall 5 SW.png");

	walk_w1 = loadImage("Media/Walkways/Walkway 1 W.png");
	walk_e1 = loadImage("Media/Walkways/Walkway 1 E.png");
	walk_w2 = loadImage("Media/Walkways/Walkway 2 W.png");
	walk_e2 = loadImage("Media/Walkways/Walkway 2 E.png");

	totem_n1 = loadImage("Media/Totems/Totem 1 N.png");
	totem_s1 = loadImage("Media/Totems/Totem 1 S.png");
	totem_n2 = loadImage("Media/Totems/Totem 2 N.png");
	totem_s2 = loadImage("Media/Totems/Totem 2 S.png");

	cat = loadImage("Media/Other/Cat's Eyes.png");
	far_background = loadImage("Media/Other/Far Background.png");
	flare = loadImage("Media/Other/Flare.png");
	grass = loadImage("Media/Other/Grass.png");
	near_background = loadImage("Media/Other/Near Background.png");
	roots = loadImage("Media/Other/Roots.png");
#pragma endregion Texture Loading
}

SDL_Texture* LevelManager::loadImage(const char *filename)
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( filename );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError() );
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	return newTexture;
}

void LevelManager::initializeLevels()
{
	//Home level
	home_texelw = 40;
	home_texelh = 40;

	home_level_width = 65;
	home_level_height = 12;
	home_level_size = home_level_width*home_level_height;

	home_level_background[0] = far_background;
	home_level_background[1] = NULL;
	textures home_level_blueprint[780] = {
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
        NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NW1, NE2, NW2, NE1, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NW3,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, SW3, SE4, SW4, SE3, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, SW3,
		NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NW3, NE4, NW4, NE3, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NW3,
		MCH, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, SOR, NON, NON, NON, NON, NON, NON, SW3, SE4, SW4, SE3, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, NON, SW3,
		NW1, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW5, NE4, NW4, NE5, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW2, NE2, NW3 };

	buildLevel(home_level_blueprint, home_level, home_level_height, home_level_width, home_texelw, home_texelh);

}

void LevelManager::loadLevel(level_type level)
{
	current_level = level;
	switch (current_level)
	{
	case HOME_LEVEL:
		current_width = home_level_width;
		current_height = home_level_height;
		texel_width = home_texelw;
		texel_height = home_texelh;
		break;
	}
}

void LevelManager::buildLevel(textures blueprint[], Tile *level_boxes[], int height, int width, int tex_width, int tex_height)
{
	for (int i = 0; i < height*width; i++)
	{
		bool solid = true;
		SDL_Texture *level;

		switch (blueprint[i])
		{
#pragma region tiles
		case NE1:
			level = wall_ne1;
			break;
		case NW1:
			level = wall_nw1;
			break;
		case SE1:
			level = wall_se1;
			break;
		case SW1:
			level = wall_sw1;
			break;
		case NE2:
			level = wall_ne2;
			break;
		case NW2:
			level = wall_nw2;
			break;
		case SE2:
			level = wall_se2;
			break;
		case SW2:
			level = wall_sw2;
			break;
		case NE3:
			level = wall_ne3;
			break;
		case NW3:
			level = wall_nw3;
			break;
		case SE3:
			level = wall_se3;
			break;
		case SW3:
			level = wall_sw3;
			break;
		case NE4:
			level = wall_ne4;
			break;
		case NW4:
			level = wall_nw4;
			break;
		case SE4:
			level = wall_se4;
			break;
		case SW4:
			level = wall_sw4;
			break;
		case NE5:
			level = wall_ne5;
			break;
		case NW5:
			level = wall_nw5;
			break;
		case SE5:
			level = wall_se5;
			break;
		case SW5:
			level = wall_sw5;
			break;
		case WE1:
			level = walk_e1;
			break;
		case WW1:
			level = walk_w1;
			break;
		case WE2:
			level = walk_e2;
			break;
		case WW2:
			level = walk_w2;
			break;
		case TN1:
			level = totem_n1;
			solid = false;
			break;
		case TS1:
			level = totem_s1;
			solid = false;
			break;
		case TN2:
			level = totem_n2;
			solid = false;
			break;
		case TS2:
			level = totem_s2;
			solid = false;
			break;
		case GRA:
			level = grass;
			solid = false;
			break;
		case ROT:
			level = roots;
			solid = false;
			break;
		case NON:
			level = NULL;
			break;
#pragma endregion tiles
		case MCH:
			level = NULL;
			start_x = (i%width)*tex_width;
			start_y = (i/width)*tex_height;
			break;
		case SOR:
			level = NULL;
			EnemyMang.addSmallOrc((i%width)*tex_width, (i/width)*tex_height);
			break;
		}

		if (level == NULL)
			level_boxes[i] = NULL;
		else
			level_boxes[i] = new Tile((i%width)*tex_width, (i/width)*tex_height, tex_width, tex_height, level, solid);
	}
}

void LevelManager::draw()
{
	if (current_level == HOME_LEVEL)
	{
		for (int i = 0; i < 2; i++)
		{
			if (home_level_background[i] == NULL)
				continue;
			SDL_RenderCopy(renderer, home_level_background[i], NULL, background);
		}
		for (int i = 0; i < home_level_width*home_level_height; i++)
		{
			if (home_level[i] == NULL)
				continue;
			home_level[i]->draw(Camera, renderer);
		}
	}
}

int LevelManager::getWidth()
{
	return current_width*texel_width;
}

int LevelManager::getHeight()
{
	return current_height*texel_height;
}

int LevelManager::getSize()
{
	return current_width*current_height;
}

SDL_Rect LevelManager::getBoxes(int i)
{
	if (home_level[i] != NULL)
		return home_level[i]->getBox();
	SDL_Rect temp =  { 0, 0, 0, 0 };
	return temp;
}

Tile** LevelManager::getLevel()
{
	if (current_level == HOME_LEVEL)
		return home_level;
	return NULL;
}
#pragma endregion LevelManager Class

//Level Manager
LevelManager LevelMang;


#pragma region Main
//Initializes SDL, the window, renderer, and screen (Read LazyFoo)
bool init()
{
	if (SDL_Init( SDL_INIT_EVERYTHING ) == -1)
		return false;

	window = SDL_CreateWindow("Ninja Guy: Den",
                          SDL_WINDOWPOS_CENTERED,
                          SDL_WINDOWPOS_CENTERED,
                          SCREEN_WIDTH, SCREEN_HEIGHT, 0);
	renderer = SDL_CreateRenderer(window, -1, 0);

	screen = SDL_CreateTexture(renderer,
                               SDL_PIXELFORMAT_ARGB8888,
                               SDL_TEXTUREACCESS_STREAMING,
                               640, 480);

	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, 640, 480);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);

	if (window == NULL)
		return false;

	return true;
}

//Use to load a new image (Pass in the filename as a string. Suggested: Put any media in the media folder and begin any file name with "Media/")
SDL_Texture* loadImage(const char *filename)
{
	//The final texture
	SDL_Texture* newTexture = NULL;

	//Load image at specified path
	SDL_Surface* loadedSurface = IMG_Load( filename );
	if( loadedSurface == NULL )
	{
		printf( "Unable to load image %s! SDL_image Error: %s\n", filename, IMG_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        newTexture = SDL_CreateTextureFromSurface( renderer, loadedSurface );
		if( newTexture == NULL )
		{
			printf( "Unable to create texture from %s! SDL Error: %s\n", filename, SDL_GetError() );
		}

		//Get rid of old loaded surface
		SDL_FreeSurface( loadedSurface );
	}

	return newTexture;
}

//Loads all the files
bool loadFiles()
{
	EnemyMang.load();

	LevelMang.loadTextures();
	LevelMang.initializeLevels();

	Player.load();
	Player.init(start_x, start_y);

	return true;
}

void setCamera(SDL_Rect box, int width, int height)
{
	Camera.x = (box.x + box.w / 2) - SCREEN_WIDTH / 2; //1
	Camera.y = (box.y + box.h / 2) - SCREEN_HEIGHT / 2;

	//Keep the camera in bounds
	if (Camera.x < 0)
	{
		Camera.x = 0;
	}
	if (Camera.y < 0)
	{
		Camera.y = 0;
	}
	if (Camera.x > width - Camera.w)
	{
		Camera.x = width - Camera.w;
	}
	if (Camera.y > height - Camera.h)
	{
		Camera.y = height - Camera.h;
	}
}

void cleanUp()
{
	SDL_Quit();
}

void collisionManager()
{
	Player.checkCollision();
}

void update()
{
	CurrentLevel = LevelMang.getLevel();
	LevelWidth = LevelMang.getWidth();
	LevelHeight = LevelMang.getHeight();
	LevelSize = LevelMang.getSize();

	collisionManager();

	Player.move();

	EnemyMang.update();

	setCamera(Player.getBox(), LevelMang.getWidth(), LevelMang.getHeight());
}

//Draws the screen and any additional things *** MAKE SURE TO PUT ANY DRAWING AFTER THE FIRST TWO LINES
void draw()
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);

	LevelMang.draw();
	Player.show(Camera.x, Camera.y);
	EnemyMang.draw();

	//Health is always last
	Player.drawHealth();

	SDL_RenderPresent(renderer);
}
#pragma endregion Main Functions

int main( int argc, char* args[] )
{
	//Main quit variable
    bool quit = false;

	//FPS
	frame = 0;

	//Initialize everything
	if (init() == false)
		return 1;

	//Load all files
	if (loadFiles() == false)
		return 1;

	//Initialize to home level
	LevelMang.loadLevel(LevelMang.HOME_LEVEL);

	//Main game loop
	while (!quit)
	{
		fps.start();
		while (SDL_PollEvent(&event))
		{
			Player.handleInput(event);

			//Quit options
			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
					quit = true;
			}
			if (event.type == SDL_QUIT) //X's out the window
				quit = true;
		}

		update();

		draw();

		//Regulates frame rate
		frame++;

		if (fps.getTicks() < 1000/FPS)
			SDL_Delay((1000/FPS) - fps.getTicks());

	}
    return 0;    
}