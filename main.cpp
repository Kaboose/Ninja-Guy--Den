#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "SDL_mixer.h"
#include "Timer.h"
#include "Tile.h"
#include <string>
#include <sstream>
#include <vector>
#include <fstream>

using namespace std;

ifstream input_file; //scripts
ifstream load_game;
ofstream save_game;

//Main quit variable
bool quit = false;

typedef enum { PLAYING, AI_SPEAKING, PAUSE_MENU, MAIN_MENU } GAME_STATES;
GAME_STATES GAME_STATE = MAIN_MENU;

//Levels
typedef enum { MENU_LEVEL, HOME_LEVEL, LEVEL_ONE } level_type;
level_type current_level;

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

//Text
SDL_Color textColor = {255,255,255};
TTF_Font *gFont = NULL;
TTF_Font *scriptFont = NULL;

//TextBox
SDL_Texture *textArea = NULL;

//Music
Mix_Music *music = NULL;

//Sounds
Mix_Chunk *door = NULL;
Mix_Chunk *ogre = NULL;
Mix_Chunk *swing = NULL;
Mix_Chunk *clank = NULL;
Mix_Chunk *chest = NULL;
Mix_Chunk *fire = NULL;
Mix_Chunk *burr = NULL;
Mix_Chunk *lizard = NULL;
Mix_Chunk *ice = NULL;
Mix_Chunk *earth = NULL;
Mix_Chunk *air = NULL;

//Read the LazyFoo tutorials to better understand
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL; //<-Use to render
SDL_Texture* screen; //<-Render to

SDL_Event event;

Timer fps;

//Current Level
Tile** CurrentLevel;
SDL_Texture** Background;
int LevelSize;
int LevelWidth;
int LevelHeight;

#pragma region Global Functions
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

SDL_Texture* loadText( std::string textureText, int value, TTF_Font* font, SDL_Rect* box )
{
	SDL_Texture* texture = NULL;
	std::stringstream stream;

	//Render text surface
	if (value == -1)
		stream << textureText;
	else
		stream << textureText << value;

	SDL_Surface* textSurface = TTF_RenderText_Solid( font, stream.str().c_str(), textColor );
	if( textSurface == NULL )
	{
		printf( "Unable to render text surface! SDL_ttf Error: %s\n", TTF_GetError() );
	}
	else
	{
		//Create texture from surface pixels
        texture = SDL_CreateTextureFromSurface( renderer, textSurface );
		if( texture == NULL )
		{
			printf( "Unable to create texture from rendered text! SDL Error: %s\n", SDL_GetError() );
		}
		else
		{
			box->w = textSurface->w;
			box->h = textSurface->h;
			box->x -= box->w/2;
			box->y -= box->h;
		}

		//Get rid of old surface
		SDL_FreeSurface( textSurface );
	}
	
	//Return success
	return texture;
}
#pragma endregion Global Functions



//******************************************** WEAPONS ************************************************

#pragma region Weapons
//NinjaStar Class and Functions
#pragma region NinjaStar
class NinjaStar
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

	int damage;

	SDL_RendererFlip flip;

	SDL_Rect* clip;
};

void NinjaStar::init(SDL_Texture* texture, SDL_Rect box, float xvel, float yvel)
{
	Mix_PlayChannel(-1, clank, 0);
	this->texture = texture;
	this->box = box;
	this->xvel = xvel;
	this->yvel = yvel;
	angle = 0;
	flip = SDL_FLIP_NONE;
	clip = NULL;
}

void NinjaStar::update()
{
	box.x += xvel;
	box.y += yvel;
	angle += 50;
	if (xvel == 0 && yvel == 0)
		angle = 0;
}

void NinjaStar::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RenderCopyEx(renderer, texture, clip, &destination, angle, NULL, flip);
}

SDL_Rect NinjaStar::getBox()
{
	return box;
}
#pragma endregion NinjaStar Class

//Spell Class
#pragma region Spell
class Spell : public NinjaStar
{
public:
	typedef enum { FIRE, WIND, ICE, EARTH } spell_type;
	void init(SDL_Texture* texture, int x, int y, float xvel, float yvel, SDL_RendererFlip flip, spell_type type, SDL_Rect* clip);
	void update();

	spell_type type;
};

void Spell::init(SDL_Texture* texture, int x, int y, float xvel, float yvel, SDL_RendererFlip flip, spell_type type, SDL_Rect* clip)
{

	if(type == FIRE)
	    Mix_PlayChannel(-1, fire, 0);
	else if(type == WIND)
		Mix_PlayChannel(-1, air, 0);
	else if(type == ICE)
		Mix_PlayChannel(-1, ice, 0);
	else if(type == EARTH)
	    Mix_PlayChannel(-1, earth, 0);




	this->texture = texture;
	this->xvel = xvel;
	this->yvel = yvel;
	this->flip = flip;
	this->type = type;
	this->clip = clip;

	SDL_Rect tempBox = { x, y, 30, 30 };
	box = tempBox;

	damage = 3;
}

void Spell::update()
{
	box.x += xvel;
	box.y += yvel;
}
#pragma endregion Spell Class

//Sword Class and Functinos
#pragma region Sword
class Sword
{
public:
	typedef enum { LEFT = SDL_FLIP_NONE, RIGHT = SDL_FLIP_HORIZONTAL } direction;

	void init(SDL_Texture* texture, SDL_Rect box, SDL_RendererFlip flip);
	void update(SDL_Rect player, SDL_RendererFlip flip);
	void draw();
	SDL_Rect getBox();

	SDL_Texture* texture;
	SDL_Rect box;
	int angle;
	int start;

	int leftAngles[3];
	int rightAngles[3];
	int index;

	int damage;

	SDL_RendererFlip flip;

	bool done;
};

void Sword::init(SDL_Texture* texture, SDL_Rect box, SDL_RendererFlip flip)
{
	Mix_PlayChannel(-1, swing, 0);
	start = frame;
	this->texture = texture;
	this->flip = flip;
	SDL_Rect temp = { box.x+box.w/2, box.y+box.h/2, 60, 10 };
	if (flip == SDL_FLIP_NONE)
		temp.x -= temp.w;
	this->box = temp;

	index = 0;
	leftAngles[0] = -300;
	leftAngles[1] = -360;
	leftAngles[2] = leftAngles[0];

	rightAngles[0] = -60;
	rightAngles[1] = 0;
	rightAngles[2] = rightAngles[0];

	if (flip == RIGHT)
		angle = rightAngles[index];
	else
		angle = leftAngles[index];

	done = false;
}

void Sword::update(SDL_Rect player, SDL_RendererFlip flip)
{
	if ((frame+1 - start) % 10 == 0)
	{
		if (index < 2)
			index++;
		else
			done = true;
	}

	if (flip == RIGHT)
		angle = rightAngles[index];
	else
		angle = leftAngles[index];

	box.x = player.x + player.w/2;
	box.y = player.y + player.h/2;
	if (flip == SDL_FLIP_NONE)
		box.x -= box.w;

	this->flip = flip;
}

void Sword::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };

	SDL_Point center = {0, destination.h/2};
	if (flip == LEFT)
	{
		center.x = destination.w;
	}

	SDL_RenderCopyEx(renderer, texture, NULL, &destination, angle, &center, flip);
}

SDL_Rect Sword::getBox()
{
	return box;
}
#pragma endregion Sword Class
#pragma endregion Weapons

//******************************************* END WEAPONS *****************************************




//******************************** MOVEABLE WITH SUBCLASSES AND MANAGER ********************************

#pragma region Moveables
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
	SDL_Texture *sword_texture;
	SDL_Texture* health_bar;
	SDL_Texture* mana_bar;

	//Magic textures
	SDL_Texture *fire;
	SDL_Texture *ice;
	SDL_Texture *rock;
	SDL_Texture *magic;

	//Magic rectangles
	SDL_Rect tornadoClip;

	int beginFrame; //Used to know when buttons were pressed to regulate animation properly
	int lastThrow;
	int lastSwing;
	int lastA;
	int lastD;

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
	bool swinging;
	bool casting;
	bool actionCheck;

	//NinjaStar
	NinjaStar ninja_star;
	int numNinjaStars;

	//Sword
	Sword sword;

	//Health/Mana
	int health, mana;
	SDL_Rect health_box, mana_box;

	//Magic
	int magicIndex;
	int magicCapacity;
	typedef enum { FIRE, ICE, EARTH, WIND } spells;
	spells magicArray[4];
	Spell casted_spell;

public:
	Character(); //Constructor
	void init(int x, int y); //Initializes at a specific x,y location
	void load(); //Loads character texture
	void move(); //Moves the character
	void handleInput(SDL_Event event); //Handles the input
	void show(int x, int y); //Draws the character on the screen

	void save();

	bool checkAction();
	void resetActionCheck();
	void movingCollision();
	bool collision[3];

	//Ninja star
	void addNinjaStars(int amount);
	SDL_Rect getNinjaStarBox();
	void delNinjaStar();
	int ninjaStarDamage();

	//Sword
	SDL_Rect getSwordBox();
	void delSword();
	int swordDamage();

	//Spells
	SDL_Rect getSpellBox();
	void delSpell();
	int spellDamage();
	Spell::spell_type spellType();

	//Health/Mana
	void drawStats();
	void damage(int amount);
	void cast(int amount);
	void addHealth(int amount);
	void addMana(int amount);

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
	lastThrow = 0;
	lastA = -30;
	lastD = -30;

	jumping = false;
	falling = false;
	jumpable = true;
	walkLeft= false;
	walkRight = false;
	walkable = true;
	stop = true;
	running = false;
	throwing = false;
	swinging = false;
	casting = false;
	actionCheck = false;

	//Health/Mana
	health = 100;
	SDL_Rect temp = { 10, 50, health*3, 10 };
	health_box = temp;
	mana = 100;
	SDL_Rect temp2 = { 10, 70, mana*3, 10 };
	mana_box = temp2;

	//Weapons
	ninja_star.damage = 4;
	numNinjaStars = 0;

	sword.damage = 6;

	//Magic
	SDL_Rect tempTornado = { 72, 176, 40, 40 };
	tornadoClip = tempTornado;
	magicIndex = 0;
	magicCapacity = 4;
	spells tempMagicArray[] = { FIRE, ICE, EARTH, WIND };
	for (int i = 0; i < 4; i++)
		magicArray[i] = tempMagicArray[i];
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
	sword_texture = loadImage("Media/Objects/knife.png");
	health_bar = loadImage("Media/Health.png");
	mana_bar = loadImage("Media/Mana.png");
	
	//Magic
	fire = loadImage("Media/Objects/fireball.png");
	rock = loadImage("Media/Objects/rock.png");
	ice = loadImage("Media/Objects/snowflake.png");
	magic = loadImage("Media/Objects/magic_icons.png");
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

				//Running
				if (frame - lastA < 20)
					running = true;
				lastA = frame;
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

				//Running
				if (frame - lastD < 20)
					running = true;
				lastD = frame;
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

				swinging = false;
			}
			break;
		case SDLK_e:
			if (event.key.repeat == 0)
				actionCheck = true;
			break;
		case SDLK_f:
			if (event.key.repeat == 0 && mana > 0 && !casting)
			{
				SDL_RendererFlip flip;
				float xvel = 0;
				if (direction == FACE_RIGHT)
				{
					xvel = 10;
					flip = SDL_FLIP_NONE;
				}
				else
				{
					xvel = -10;
					flip = SDL_FLIP_HORIZONTAL;
				}

				switch(magicArray[magicIndex])
				{
				case FIRE:
					casted_spell.init(fire, box.x, box.y, xvel, 0, flip, Spell::FIRE, NULL);
					break;
				case ICE:
					casted_spell.init(ice, box.x, box.y, xvel, 0, flip, Spell::ICE, NULL);
					break;
				case EARTH:
					casted_spell.init(rock, box.x, box.y, xvel, 0, flip, Spell::EARTH, NULL);
					break;
				case WIND:
					casted_spell.init(magic, box.x, box.y, xvel, 0, flip, Spell::WIND, &tornadoClip);
					break;
				}
				cast(10);
				casting = true;
			}
			break;
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
					running = false;
				}
				break;
            case SDLK_d: 
				if (walkRight && !walkLeft) 
				{
					if (!jumping)
						currentClip = 8;
					walkRight = false;
					running = false;
				}
				break;
        }        
    }

	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		if (event.button.button == SDL_BUTTON_RIGHT && numNinjaStars > 0)
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
				lastThrow = frame;
				numNinjaStars--;
			}
		}
		else if (event.button.button == SDL_BUTTON_LEFT &&!swinging)
		{
			SDL_RendererFlip flip;
			if (direction == FACE_RIGHT)
				flip = SDL_FLIP_HORIZONTAL;
			else
				flip = SDL_FLIP_NONE;

			sword.init(sword_texture, box, flip);
			swinging = true;
			lastSwing = frame;
		}
	}

	if (event.type == SDL_MOUSEWHEEL)
	{
		if (event.wheel.y > 0)
		{
			magicIndex++;
			if (magicIndex > magicCapacity-1)
				magicIndex = 0;
		}
		else if (event.wheel.y < 0)
		{
			magicIndex--;
			if (magicIndex < 0)
				magicIndex = magicCapacity-1;
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
		if (yvel > MAX_Y_VELOCITY)
			yvel = MAX_Y_VELOCITY;

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
				xvel = -(X_VELOCITY+2);
		}
		else
		{
			xvel = 0;
		}
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
				xvel = X_VELOCITY+2;
		}
		else
		{
			xvel = 0;
		}
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
	mana_box.w = mana*3;

	//Ninja star
	if (ninja_star.texture != NULL)
		ninja_star.update();

	if (frame - lastThrow > 80)
	{
		throwing = false;
		delNinjaStar();
	}

	//Spell
	if (casting)
		casted_spell.update();

	if (!checkCollision(casted_spell.getBox(), Camera))
	{
		delSpell();
		casting = false;
	}

	//Sword
	SDL_RendererFlip flip = SDL_FLIP_NONE;
	if (direction == FACE_RIGHT)
		flip = SDL_FLIP_HORIZONTAL;

	if (swinging)
		sword.update(box, flip);

	if (sword.done)
		delSword();
	if (frame+1 - lastSwing > 70)
	{
		swinging = false;
		walkable = true;
	}

	//STOP PLAYER
	if (GAME_STATE != PLAYING)
	{
		walkLeft = false;
		walkRight = false;
	}
}

void Character::show(int x, int y)
{
	if (ninja_star.texture != NULL)
		ninja_star.draw();

	if (casting)
		casted_spell.draw();

	SDL_Rect destination = { box.x - x, box.y - y, box.w, box.h };

	//LazyFoo** must pass in the renderer, texture, reference to the sprite (as a rectangle), and reference to the destination on the screen
	if (GAME_STATE != MAIN_MENU)
		SDL_RenderCopy(renderer, texture, &clip[currentClip], &destination);

	if (swinging)
		sword.draw();
}

void Character::save()
{
	save_game << magicCapacity << "\n";
	for (int i = 0; i < magicCapacity; i++)
		save_game << magicArray[i] << "\n";
}

bool Character::checkAction()
{
	return actionCheck;
}

void Character::resetActionCheck()
{
	actionCheck = false;
}

void Character::movingCollision()
{
	int index = (box.y/40)*LevelWidth/40 + (box.x/40);
	if (index >= LevelSize || index <= 0)
	{
		collision[LEFT] = false;
		collision[RIGHT] = false;
	}
	else 
	{
		if (CurrentLevel[index-1] == NULL || (index*40)%LevelWidth == 0)
			collision[LEFT] = false;
		else if (box.x <= CurrentLevel[index-1]->getBox().x+CurrentLevel[index-1]->getBox().w + 4)
			collision[LEFT] = CurrentLevel[index-1]->collidable();
		
		if (CurrentLevel[index+1] == NULL || index%(LevelWidth/40) == LevelWidth/40 - 1)
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
				if (CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40] != NULL)
				{
					SDL_Rect tile = CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40]->getBox();
					if (box.x < tile.x + tile.w && CurrentLevel[index+LevelWidth/40*row+(xvel*row)/40]->collidable())
					{
						ground_tile = tile;
						collision[BOTTOM] = true;
					}
				}
				else if (CurrentLevel[index +1 +LevelWidth/40*row+(xvel*row)/40] != NULL &&  index%(LevelWidth/40) != LevelWidth/40 - 1)
				{
					SDL_Rect tile = CurrentLevel[index +1 +LevelWidth/40*row+(xvel*row)/40]->getBox();
					if (box.x + box.w-5 > tile.x && CurrentLevel[index +1 +LevelWidth/40*row+(xvel*row)/40]->collidable())
					{
						ground_tile = tile;
						collision[BOTTOM] = true;
					}
				}
				row++;
			}
		}
	}

	//Ninja star
	index = (ninja_star.box.y/40)*LevelWidth/40 + (ninja_star.box.x/40);
	if (index >= LevelSize || index < 0 || ninja_star.texture == NULL || CurrentLevel[index] == NULL)
		return;

	if (CurrentLevel[index]->collidable())
	{
		ninja_star.xvel = 0;
		ninja_star.yvel = 0;
	}
}

void Character::addNinjaStars(int amount)
{
	numNinjaStars += amount;
}

SDL_Rect Character::getNinjaStarBox()
{
	return ninja_star.box;
}

void Character::delNinjaStar()
{
	ninja_star.texture = NULL;
	ninja_star.box.x = Camera.x-100;
	ninja_star.box.y = Camera.y-100;
}

int Character::ninjaStarDamage()
{
	return ninja_star.damage;
}

SDL_Rect Character::getSwordBox()
{
	return sword.box;
}

void Character::delSword()
{
	sword.texture = NULL;
	sword.box.x = Camera.x-100;
	sword.box.y = Camera.y-100;
}

int Character::swordDamage()
{
	return sword.damage;
}

SDL_Rect Character::getSpellBox()
{
	return casted_spell.box;
}

void Character::delSpell()
{
	casted_spell.texture = NULL;
	casted_spell.box.x = Camera.x-100;
	casted_spell.box.y = Camera.y-100;
}

int Character::spellDamage()
{
	return casted_spell.damage;
}

Spell::spell_type Character::spellType()
{
	return casted_spell.type;
}

void Character::drawStats()
{
	if (GAME_STATE != MAIN_MENU){
		//NinjaStar
		SDL_Rect temp = { 10, 10, 30, 30 };
		SDL_RenderCopy(renderer, ninja_star_texture, NULL, &temp);
		SDL_Rect temp2 = { 45, 50, 15, 15 };
		SDL_Texture* number = loadText("", numNinjaStars, scriptFont, &temp2);
		SDL_RenderCopy(renderer, number, NULL, &temp2);

		//Health/Mana
		SDL_RenderCopy(renderer, health_bar, NULL, &health_box);
		SDL_RenderCopy(renderer, mana_bar, NULL, &mana_box);

		//Magic
		if (magicCapacity > 0)
		{
			SDL_Rect magic_destination = { 60, 10, 30, 30 };
			switch (magicArray[magicIndex])
			{
			case FIRE:
				SDL_RenderCopy(renderer, fire, NULL, &magic_destination);
				break;
			case ICE:
				SDL_RenderCopy(renderer, ice, NULL, &magic_destination);
				break;
			case EARTH:
				SDL_RenderCopy(renderer, rock, NULL, &magic_destination);
				break;
			case WIND:
				SDL_RenderCopy(renderer, magic, &tornadoClip, &magic_destination);
				break;
			}
		}
	}
}

void Character::damage(int amount)
{
	health -= amount;
}

void Character::cast(int amount)
{
	mana -= amount;
}

void Character::addHealth(int amount)
{
	health += amount;
}

void Character::addMana(int amount)
{
	mana += amount;
}
#pragma endregion Character Class

//******THE PLAYER**********
Character Player;


//Enemy Class and Functions
#pragma region Enemy
class Enemy : public Moveable
{
public:
	typedef enum { STANDING_LEFT, STANDING_RIGHT, RUNNING_LEFT, RUNNING_RIGHT, JUMPING, RECOVER, DEAD} states;
	typedef enum { LEFT = 0, RIGHT, BOTTOM } sides;
	typedef enum { SMALL_ORC, GRIZZOLAR_BEAR, BIRD, LIZARD } types;

	types type;

	Enemy(SDL_Texture* texture, int x, int y, types type);
	void update();
	void draw();

	SDL_Rect getFireBox();
	int getFireDamage();
	void delFireball();

	void setState(states state);
	int getState();

	bool collision[3];
	int health;

	states aggroState;

private:
	states state;
	SDL_RendererFlip flip;
	int angle;

	SDL_Texture* texture;
	SDL_Texture* fire;
	Spell fireball;
	SDL_Rect clip[12];
	int currentClip;
	int index;
	int beginFrame;
	int lastFired;

	bool falling;
	bool firing;
};

Enemy::Enemy(SDL_Texture *texture, int x, int y, types type)
{
	this->type = type;

	if (type == SMALL_ORC)
	{
		state = STANDING_LEFT;
		X_VELOCITY = 4;
		Y_VELOCITY = 1;
		MAX_Y_VELOCITY = 10;
	
		index = 1;
		angle = 0;
		health = 10;
	
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
		box.y = y - (box.h - 45);

		currentClip = 0;
	}
	else if (type == GRIZZOLAR_BEAR)
	{
		state = STANDING_LEFT;
		X_VELOCITY = 5;
		Y_VELOCITY = 1;
		MAX_Y_VELOCITY = 10;

		index = 1;
		angle = 0;
		health = 10;
	
		int idle_width = 56, idle_height = 64;
		int run_width = 42, run_height = 64;
		int jump_width = 56, jump_height = 64;
		int standing_gap = 16, running_gap = 46, jumping_gap = 40;
	
		for (int i = 0; i < 12; i++)
		{
			if (i < 4)
			{
				clip[i].x = i%4*(idle_width+standing_gap)+standing_gap;
				clip[i].y = 120;
				clip[i].w = idle_width;
				clip[i].h = idle_height;
				continue;
			}
			else if (i < 8)
			{
				clip[i].x = i%4*(run_width+running_gap) + 24;
				clip[i].y = 200;
				clip[i].w = run_width;
				clip[i].h = run_height;
			}
			else
			{
				clip[i].x = i%4*(jump_width*jumping_gap) + 24;
				clip[i].y = 472;
				clip[i].w = jump_width;
				clip[i].h = jump_height;
			}
		}
	
		box.w = idle_width;
		box.h = idle_height;
		box.y = y - (box.h - 45);

		currentClip = 0;
	}
	else if (type == LIZARD)
	{
		state = STANDING_LEFT;
		X_VELOCITY = 2;
		Y_VELOCITY = 1;
		MAX_Y_VELOCITY = 10;
	
		index = 1;
		angle = 0;
		health = 10;
	
		int idle_width = 32, idle_height = 32;
	
		for (int i = 0; i < 3; i++)
		{
			clip[i].x = i%3*idle_width;
			clip[i].y = 64;
			clip[i].w = idle_width;
			clip[i].h = idle_height;
		}
	
		box.w = 50;
		box.h = 50;
		box.y = y-10;

		currentClip = 1;

		fire = loadImage("Media/Objects/fireball.png");
	}
	
	this->texture = texture;
	box.x = x;

	xvel = 0;
	yvel = 0;

	collision[LEFT] = false;
	collision[RIGHT] = false;

	ground_tile.w = 40;
	ground_tile.h = 40;

	falling = false;
	firing = false;
}

void Enemy::update()
{
	if (state == STANDING_LEFT || state == STANDING_RIGHT || state == RECOVER)
	{
		if (type == GRIZZOLAR_BEAR || type == SMALL_ORC)
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
				setState(aggroState);
		}

		if (type == LIZARD)
		{
			xvel = 0;
			if (state == RECOVER && (frame-beginFrame+1)%75 == 0)
				setState(aggroState);
		}
	}

	if (state == RUNNING_LEFT || state == RUNNING_RIGHT)
	{
		switch (type)
		{
		case SMALL_ORC:		
			if ((frame-beginFrame)%4 == 0)
				currentClip++;

			if (currentClip > 11)
				currentClip = 4;
			break;
		case GRIZZOLAR_BEAR:
			if ((frame-beginFrame)%4 == 0)
				currentClip += index;

			if (currentClip > 7 || currentClip < 4)
			{
				index *= -1;
				currentClip += index;
			}
			break;
		case LIZARD:
			if ((frame-beginFrame)%4 == 0)
				currentClip += index;

			if (currentClip < 0 || currentClip > 2)
			{
				index *= -1;
				currentClip += index;
			}

			if (!firing && (frame+1 - beginFrame)%60 == 0)
			{
				int speed;
				(state == RUNNING_LEFT ? speed = -10 : speed = 10);
				fireball.init(fire, box.x, box.y, speed, 0, flip, Spell::FIRE, NULL);
				lastFired = frame;
				firing = true;
			}
			break;
		}
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
			setState(STANDING_LEFT);
		}

		if (collision[RIGHT] && state == RUNNING_RIGHT)
		{
			setState(STANDING_RIGHT);
		}
	}

	if (state == JUMPING)
	{
		if (currentClip < 10 && frame-beginFrame%2 == 0)
			currentClip++;
		if (yvel >= 0)
			falling = true;
	}

	if (falling)
	{
		yvel += Y_VELOCITY;
		if (collision[BOTTOM])
		{
			if (((box.x >= ground_tile.x && box.x <= ground_tile.x + ground_tile.w) ||
				(box.x <= ground_tile.x && (box.x + box.w) >= ground_tile.x)) &&
				box.y >= ground_tile.y - 75)
			{
				box.y = ground_tile.y - box.h+5;
				yvel = 0;
				falling = false;
			}
		}
	}

	if (firing)
	{
		fireball.update();
		if ((frame+1 - lastFired)%150 == 0)
			firing = false;
	}

	if (state == DEAD)
	{
		if( type == SMALL_ORC)
			Mix_PlayChannel(-1, ogre, 0);		
		else if( type == LIZARD )
			Mix_PlayChannel(-1, lizard, 0);
		else if( type == GRIZZOLAR_BEAR)
			Mix_PlayChannel(-1, burr, 0);
		yvel += 1;
		angle += 10;
	}

	if (state == STANDING_LEFT || state == RUNNING_LEFT)
		flip = SDL_FLIP_HORIZONTAL;
	else
		flip = SDL_FLIP_NONE;

	if (type != LIZARD)
	{
		box.w = clip[currentClip].w;
		box.h = clip[currentClip].h;
	}

	box.x += xvel;
	box.y += yvel;

	if (ground_tile.y - box.y > box.h+5)
		falling = true;
}

void Enemy::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RenderCopyEx(renderer, texture, &clip[currentClip], &destination, angle, NULL, flip);

	if (firing)
		fireball.draw();
}

SDL_Rect Enemy::getFireBox()
{
	return fireball.getBox();
}

int Enemy::getFireDamage()
{
	return fireball.damage;
}

void Enemy::delFireball()
{
	firing = false;
	fireball.texture = NULL;
	fireball.box.x = Camera.x-100;
	fireball.box.y = Camera.y-100;
}

void Enemy::setState(states state)
{
	if (this->state == DEAD)
		return;

	this->state = state;
	beginFrame = frame;
	if (state == STANDING_LEFT || state == STANDING_RIGHT || state == RECOVER)
	{
		if (type == SMALL_ORC || type == GRIZZOLAR_BEAR)
			currentClip = 0;
		
		if (type == LIZARD)
			currentClip = 1;

		xvel = 0;
	}
	else if (state == JUMPING)
		currentClip = 8;
	else
	{
		if (type == SMALL_ORC || type == GRIZZOLAR_BEAR)
			currentClip = 4;
		
		if (type == LIZARD)
			currentClip = 1;
	}

	if (state == DEAD)
		yvel = Y_VELOCITY;
}

int Enemy::getState()
{
	return state;
}
#pragma endregion Enemy Class


//EnemyManager Class and Functions
#pragma region EnemyManager
class EnemyManager
{
private:
	//Enemy textures
	SDL_Texture* small_orc_texture;
	SDL_Texture* grizzolar_bear_texture;
	SDL_Texture* bird_texture;
	SDL_Texture* lizard_texture;

	SDL_Texture* fire;

	SDL_Texture* loadImage(const char *filename);
	std::vector<Enemy> enemies;
	bool cameraCollision(SDL_Rect box);
	void collisionManager(Enemy *enemy);
	bool checkWeak(Enemy::types enemy, Spell::spell_type spell);

public:
	void addEnemy(int x, int y, Enemy::types type);
	void load();
	void update();
	void draw();
	void deleteAll();
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
	grizzolar_bear_texture = loadImage("Media/Enemies/Grizzolar Bear.png");
	lizard_texture = loadImage("Media/Enemies/Lizard.png");

	fire = loadImage("Media/Objects/fireball.png");
}

void EnemyManager::addEnemy(int x, int y, Enemy::types type)
{
	SDL_Texture *texture = NULL;

	switch (type)
	{
	case Enemy::SMALL_ORC:
		texture = small_orc_texture;
		break;
	case Enemy::GRIZZOLAR_BEAR:
		texture = grizzolar_bear_texture;
		break;
	case Enemy::LIZARD:
		texture = lizard_texture;
		break;
	}

	enemies.push_back(Enemy(texture, x, y, type));
}

void EnemyManager::update()
{
	for (int i = 0; i < enemies.size(); i++)
	{
		SDL_Rect player = Player.getBox();
		SDL_Rect enemy_box = enemies.at(i).getBox();
		Enemy *enemy = &enemies.at(i);

		if (!cameraCollision(enemy_box))
			continue;

		//Move enemies
		switch (enemy->type)
		{
		case Enemy::SMALL_ORC:
			if (((player.y + player.h < enemy_box.y + enemy_box.h && player.y + player.h > enemy_box.y) ||
				(player.y < enemy_box.y + enemy_box.h && player.y > enemy_box.y)) &&
				(enemy->getState() == Enemy::STANDING_LEFT || enemy->getState() == Enemy::STANDING_RIGHT))
			{
				int delta_x = player.x - enemy_box.x;
	
				if (delta_x < 200 && delta_x > 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_RIGHT);
				else if (delta_x > -200 && delta_x < 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_LEFT);
			}
			break;
		case Enemy::GRIZZOLAR_BEAR:
			if (((player.y + player.h < enemy_box.y + enemy_box.h && player.y + player.h > enemy_box.y) ||
				(player.y < enemy_box.y + enemy_box.h && player.y > enemy_box.y)) &&
				(enemy->getState() == Enemy::STANDING_LEFT || enemy->getState() == Enemy::STANDING_RIGHT))
			{
				int delta_x = player.x - enemy_box.x;
	
				if (delta_x < 300 && delta_x > 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_RIGHT);
				else if (delta_x > -300 && delta_x < 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_LEFT);
			}
			break;
		case Enemy::LIZARD:
			if (((player.y + player.h < enemy_box.y + enemy_box.h && player.y + player.h > enemy_box.y) ||
				(player.y < enemy_box.y + enemy_box.h && player.y > enemy_box.y)) &&
				(enemy->getState() == Enemy::STANDING_LEFT || enemy->getState() == Enemy::STANDING_RIGHT))
			{
				int delta_x = player.x - enemy_box.x;
	
				if (delta_x < 300 && delta_x > 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_RIGHT);
				else if (delta_x > -300 && delta_x < 0 && enemy->getState() != Enemy::RECOVER)
					enemy->setState(Enemy::RUNNING_LEFT);
			}
			break;
		}

		collisionManager(enemy);

		//Update
		if (cameraCollision(enemy_box))
			enemy->update();

		if (enemy->getState() == Enemy::DEAD && !cameraCollision(enemy_box))
			enemies.erase(enemies.begin() + i);
	}
}

void EnemyManager::draw()
{
	for (int i = 0; i < enemies.size(); i++)
	{
		if (cameraCollision(enemies.at(i).getBox()))
			enemies.at(i).draw();
	}
}

void EnemyManager::deleteAll()
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies.erase(enemies.begin());
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

void EnemyManager::collisionManager(Enemy* enemy)
{
	if (enemy->getState() == Enemy::DEAD)
		return;

	SDL_Rect enemy_box = enemy->getBox();
	SDL_Rect player = Player.getBox();

	//Check collision for orc
	int index = (enemy_box.y/40)*LevelWidth/40 + (enemy_box.x/40);
	if (index >= LevelSize)
	{
		enemy->collision[Enemy::LEFT] = false;
		enemy->collision[Enemy::RIGHT] = false;
	}
	else 
	{
		if (CurrentLevel[index] == NULL)
			enemy->collision[Enemy::LEFT] = false;
		else
			enemy->collision[Enemy::LEFT] = CurrentLevel[index]->collidable();
					
		if (CurrentLevel[index+1] == NULL)
			enemy->collision[Enemy::RIGHT] = false;
		else
			enemy->collision[Enemy::RIGHT] = CurrentLevel[index+1]->collidable();

		enemy->collision[BOTTOM] = false;
		if (enemy->getYVel() < 0)
			enemy->collision[BOTTOM] = false;
		else
		{
			int row = 1;
			while (index+LevelWidth/40*row+(enemy->getXVel()*row)/40 < LevelSize && enemy->collision[BOTTOM] == false)
			{
				if (CurrentLevel[index+LevelWidth/40*row+(enemy->getXVel()*row)/40] != NULL &&
					CurrentLevel[index+LevelWidth/40*row+(enemy->getXVel()*row)/40]->collidable())
				{
					enemy->ground_tile = CurrentLevel[index+LevelWidth/40*row+(enemy->getXVel()*row)/40]->getBox();
					enemy->collision[BOTTOM] = true;
				}
				row++;
			}
		}
	}
		
	//Damage to player
	if (checkCollision(player, enemy_box))
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
					
		Player.setXVel(enemy_box.x <= player.x ? 4 : -4);

		enemy->setState(Enemy::RECOVER);

		if (enemy_box.x < player.x)
		{
			if (!enemy->collision[LEFT])
				enemy->box.x -= 10;
			enemy->aggroState = Enemy::RUNNING_RIGHT;
		}
		else
		{
			if (!enemy->collision[RIGHT])
				enemy->box.x += 10;
			enemy->aggroState = Enemy::RUNNING_LEFT;
		}
	}

	if (enemy->type == Enemy::LIZARD)
	{
		if (checkCollision(enemy->getFireBox(), Player.getBox()))
		{
			Player.damage(5*enemy->getFireDamage());
			enemy->delFireball();
		}
	}

	//Damage from player
	SDL_Rect NJbox = Player.getNinjaStarBox();
	SDL_Rect swordBox = Player.getSwordBox();
	SDL_Rect spellBox = Player.getSpellBox();
	bool star = checkCollision(enemy_box, NJbox);
	bool sword = checkCollision(enemy_box, swordBox);
	bool spell = checkCollision(enemy_box, spellBox);

	if ( (star || sword || spell) && enemy->getState() != Enemy::RECOVER)
	{
		enemy->setState(Enemy::RECOVER);
		if ((star && enemy_box.x < NJbox.x) || (sword && enemy_box.x < swordBox.x) || (spell && enemy_box.x < spellBox.x))
		{
			if (!enemy->collision[LEFT])
				enemy->box.x -= 10;
			enemy->aggroState = Enemy::RUNNING_RIGHT;
		}
		else
		{
			if (!enemy->collision[RIGHT])
				enemy->box.x += 10;
			enemy->aggroState = Enemy::RUNNING_LEFT;
		}
		Player.delNinjaStar();
		Player.delSpell();
		
		if (star)
			enemy->health -= Player.ninjaStarDamage();
		else if (sword)
			enemy->health -= Player.swordDamage();
		else if (spell)
		{
			if (checkWeak(enemy->type, Player.spellType()))
				enemy->health -= 2*Player.spellDamage();
			else
				enemy->health -= Player.spellDamage();
		}
		if (enemy->health <= 0)
			enemy->setState(Enemy::DEAD);
	}
}

bool EnemyManager::checkWeak(Enemy::types enemy, Spell::spell_type spell)
{
	switch (enemy)
	{
	case Enemy::SMALL_ORC:
		if (spell == Spell::FIRE)
			return true;
		break;
	case Enemy::GRIZZOLAR_BEAR:
		if (spell == Spell::WIND)
			return true;
		break;
	case Enemy::LIZARD:
		if (spell == Spell::ICE)
			return true;
		break;
	}

	return false;
}
#pragma endregion EnemyManager Class


//****THE ENEMY MANAGER****
EnemyManager EnemyMang;
#pragma endregion Moveables (with Enemy Manager)

//******************************** END MOVEABLE WITH SUBCLASSES AND MANAGER ******************************






//************************************ INTERACTION WITH AI/MENUS **************************************************

#pragma region AI
//Button Class and Functions
#pragma region Button
class Button
{
public:
	typedef enum { EXIT, RESUME, SAVE, NEW, LOAD } types;

	Button(std::string STRING, types type);
	void loadButton(int x, int y);
	void draw();

	SDL_Rect getBox();
	void action();

private:
	std::string STRING;
	SDL_Texture* texture;
	SDL_Rect box;

	types type;
};

Button::Button(std::string STRING, types type)
{
	this->STRING = STRING;
	this->type = type;
}

void Button::loadButton(int x, int y)
{
	box.x = x;
	box.y = y;

	texture = loadText(STRING, -1, gFont, &box);
}

void Button::draw()
{
	SDL_Rect destination = { box.x, box.y, box.w, box.h };
	SDL_RenderCopy(renderer, texture, NULL, &destination);
}

SDL_Rect Button::getBox()
{
	return box;
}

void Button::action()
{
	switch (type)
	{
	case EXIT:
		quit = true;
		break;
	case RESUME:
		GAME_STATE = PLAYING;
		break;
	case SAVE:
		save_game.open("SavedGame.txt");
		Player.save();
		save_game.close();
	case NEW:
		GAME_STATE = PLAYING;
		current_level = HOME_LEVEL;
	case LOAD:
		// call load function here
		GAME_STATE = PLAYING;
		current_level = HOME_LEVEL;
	}
}
#pragma endregion Button Class

//Menu Class and Functions
#pragma region Menu
class Menu
{
public:
	Menu();
	void addButton(Button button);
	void update(SDL_Event event);
	void draw();

private:
	std::vector<Button> buttons;
	SDL_Rect box;

	bool checkInside(SDL_Point point, SDL_Rect box);
};

Menu::Menu()
{
}

bool Menu::checkInside(SDL_Point point, SDL_Rect box)
{
	if (point.x < box.x || point.x > box.x + box.w || point.y < box.y || point.y > box.y + box.h)
		return false;

	return true;
}

void Menu::addButton(Button button)
{
	buttons.push_back(button);
}

void Menu::update(SDL_Event event)
{
	if (event.type == SDL_MOUSEBUTTONDOWN)
	{
		if (event.button.button == SDL_BUTTON_LEFT)
		{
			SDL_Point mouse;
			SDL_GetMouseState( &mouse.x, &mouse.y );

			for (int i = 0; i < buttons.size(); i++)
			{
				if (checkInside(mouse, buttons.at(i).getBox()))
				{
					buttons.at(i).action();
				}
			}
		}
	}
}

void Menu::draw()
{
	for (int i = 0; i < buttons.size(); i++)
	{
		buttons.at(i).draw();
	}
}
#pragma endregion Menu Class

//Pause Menu
Menu PauseMenu;
Button ExitButton("Exit", Button::EXIT);
Button ResumeButton("Resume", Button::RESUME);
Button SaveButton("Save", Button::SAVE);

//Main Menu
Menu MainMenu;
Button NewButton ("New Game", Button::NEW);
Button LoadButton ("Load Game", Button::LOAD);
Button QuitButton("Quit", Button::EXIT);


//AI Class and Functions
#pragma region AI
class AI
{
public:
	typedef enum { STANDING, SPEAKING, MENUING } states;
	typedef enum { WIZARD, ROBOT } types;

	AI();
	AI(int x, int y, SDL_Texture* texture, types type, std::vector<std::string> script);
	void handleInput(SDL_Event event);
	void update();
	void draw();

	SDL_Rect getBox();
	void setState(states state);
	states getState();
	void setMenu(Menu menu);

private:
	states state;
	types type;

	Menu menu;

	SDL_Rect box;
	SDL_Rect clip;

	std::vector<std::string> script;
	int currentLine;

	SDL_Texture* text;
	SDL_Texture *texture;

	SDL_Rect textBox;
	SDL_Rect leftBox, rightBox, areaBox;
	SDL_Rect leftClip, rightClip, areaClip;
};

AI::AI()
{
}

AI::AI(int x, int y, SDL_Texture* texture, types type, std::vector<std::string> script)
{
	state = STANDING;

	this->type = type;
	this->script = script;
	this->texture = texture;

	box.x = x;
	box.y = y-35;
	box.w = 50;
	box.h = 75;

	SDL_Rect temp = { 0, 0, 50, 0 };
	clip = temp;
	if (type == WIZARD)
		clip.h = 90;
	else if (type == ROBOT)
		clip.h = 95;

	SDL_Rect leftTemp = { 0, 0, 5, 40 };
	SDL_Rect rightTemp = { 35, 0, 5, 40 };
	SDL_Rect areaTemp = { 5, 0, 30, 40 };
	leftClip = leftTemp;
	rightClip = rightTemp;
	areaClip = areaTemp;

	leftBox.w = 5;
	leftBox.h = 40;
	rightBox.w = 5;
	rightBox.h = 40;

	currentLine = 0;
}

void AI::handleInput(SDL_Event event)
{
	if (event.type == SDL_KEYDOWN)
	{
		if (event.key.keysym.sym == SDLK_e && event.key.repeat == 0)
		{
			if (currentLine < script.size())
			{
				SDL_Rect temptext = { box.x+box.w/2, box.y, 100, 80 };
				textBox = temptext;
				text = loadText(script[currentLine], -1, scriptFont, &textBox);
				SDL_Rect tempArea = { textBox.x - 5, textBox.y - 5, textBox.w + 10, textBox.h + 10 };
				areaBox = tempArea;
				SDL_Rect tempLeft = { areaBox.x - 5, areaBox.y, leftBox.w, areaBox.h };
				SDL_Rect tempRight = {areaBox.x + areaBox.w, areaBox.y, rightBox.w, areaBox.h };
				leftBox = tempLeft;
				rightBox = tempRight;
				currentLine++;
			}
			else
			{
				setState(STANDING);
				currentLine = 0;
				GAME_STATE = PLAYING;
			}
		}
	}
}

void AI::update()
{
	if (checkCollision(Player.getBox(), box) && Player.checkAction())
	{
		setState(SPEAKING);
		GAME_STATE = AI_SPEAKING;

		SDL_Rect temptext = { box.x+box.w/2, box.y, 100, 80 };
		textBox = temptext;
		text = loadText(script[currentLine], -1, scriptFont, &textBox);
		SDL_Rect tempArea = { textBox.x - 5, textBox.y - 5, textBox.w + 10, textBox.h + 10 };
		areaBox = tempArea;
		SDL_Rect tempLeft = { areaBox.x - 5, areaBox.y, leftBox.w, areaBox.h };
		SDL_Rect tempRight = {areaBox.x + areaBox.w, areaBox.y, rightBox.w, areaBox.h };
		leftBox = tempLeft;
		rightBox = tempRight;
		currentLine++;
	}
}

void AI::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RenderCopy(renderer, texture, &clip, &destination);

	if (state == SPEAKING)
	{
		SDL_Rect tempArea = { areaBox.x - Camera.x, areaBox.y - Camera.y, areaBox.w, areaBox.h };
		SDL_Rect tempLeft = { leftBox.x - Camera.x, leftBox.y - Camera.y, leftBox.w, leftBox.h };
		SDL_Rect tempRight = { rightBox.x - Camera.x, rightBox.y - Camera.y, rightBox.w, rightBox.h };

		SDL_RenderCopy(renderer, textArea, &leftClip, &tempLeft);
		SDL_RenderCopy(renderer, textArea, &areaClip, &tempArea);
		SDL_RenderCopy(renderer, textArea, &rightClip, &tempRight);

		SDL_Rect temp = { textBox.x - Camera.x, textBox.y - Camera.y, textBox.w, textBox.h };
		SDL_RenderCopy(renderer, text, NULL, &temp);
	}
}

SDL_Rect AI::getBox()
{
	return box;
}

void AI::setState(states state)
{
	this->state = state;
}

AI::states AI::getState()
{
	return state;
}

void AI::setMenu(Menu menu)
{
	this->menu = menu;
}
#pragma endregion AI Class

//********* THE GLOBAL AI ***********
AI* SpeakingAI = NULL;

//AI Manager Class and Functions
#pragma region AIManager
class AIManager
{
private:
	std::vector<AI> bots;
	SDL_Texture* wizard_texture;
	SDL_Texture* robot_texture;
	std::vector<std::string> start_wizard_script;
	std::vector<std::string> robot_script;

	int current_AI;

	//Possible menus and buttons
	Menu shop;

public:
	void load();
	void addAI(int x, int y, AI::types type);
	void update();
	void draw();
	void deleteAll();
};

void AIManager::load()
{
	current_AI = 0;

	//Textures
	wizard_texture = loadImage("Media/wizard.png");
	robot_texture = loadImage("Media/robot.png");

	//Scripts
	input_file.open("Media/Scripts/start_wizard.txt");
	if (input_file.is_open())
	{
		while (!input_file.eof())
		{
			std::string line;
			getline(input_file, line);
			start_wizard_script.push_back(line);
		}
	}

	input_file.close();

	input_file.open("Media/Scripts/robot.txt");
	if (input_file.is_open())
	{
		while (!input_file.eof())
		{
			std::string line;
			getline(input_file, line);
			robot_script.push_back(line);
		}
	}

	input_file.close();
}

void AIManager::addAI(int x, int y, AI::types type)
{
	switch (type)
	{
	case AI::WIZARD:
		bots.push_back(AI(x, y, wizard_texture, AI::WIZARD, start_wizard_script));
		break;
	case AI::ROBOT:
		bots.push_back(AI(x, y, robot_texture, AI::ROBOT, robot_script));
		bots.at(current_AI).setMenu(shop);
		break;
	}

	current_AI++;
}

void AIManager::update()
{
	for (int i = 0; i < bots.size(); i++)
	{
		if (checkCollision(Camera, bots.at(i).getBox()))
		{
			bots.at(i).update();
			if (bots.at(i).getState() == AI::SPEAKING || bots.at(i).getState() == AI::MENUING)
				SpeakingAI = &bots.at(i);
		}
	}
}

void AIManager::draw()
{
	for (int i = 0; i < bots.size(); i++)
	{
		if (checkCollision(Camera, bots.at(i).getBox()))
			bots.at(i).draw();
	}
}

void AIManager::deleteAll()
{
	for (int i = 0; i < bots.size(); i++)
	{
		bots.erase(bots.begin() + i);
	}
}
#pragma endregion AIManager Class

//********** THE AI MANAGER ********
AIManager AIMang;
#pragma endregion AI with Interaction

//******************************************* END INTERACTION WITH AI/MENUS **************************









//********************************************** TREASURE CHESTS **********************************

#pragma region Treasure Chests
//Treasure Chest Class and Functions
#pragma region TreasureChest
class TreasureChest
{
public:
	typedef enum { NINJA_STARS, HEALTH_POTION, MANA_POTION } types;
	typedef enum { OPEN, CLOSED } states;

	TreasureChest();
	TreasureChest(int x, int y, types type, std::string text, SDL_Texture* open, SDL_Texture* closed);
	SDL_Rect getBox();
	void setState(states state);
	void update();
	void draw();

private:
	types type;
	states state;

	SDL_Texture* open_texture;
	SDL_Texture* closed_texture;
	SDL_Texture* text_texture;

	SDL_Rect box;

	std::string text;

	SDL_Rect textBox;
	SDL_Rect leftBox, rightBox, areaBox;
	SDL_Rect leftClip, rightClip, areaClip;

	int opened;
};

TreasureChest::TreasureChest(int x, int y, types type, std::string text, SDL_Texture* open, SDL_Texture* closed)
{
	SDL_Rect temp = { x, y, 40, 40 };
	box = temp;

	this->type = type;
	this->text = text;
	open_texture = open;
	closed_texture = closed;

	setState(CLOSED);

	SDL_Rect temptext = { box.x+box.w/2, box.y, 100, 80 };
	textBox = temptext;
	text_texture = loadText(text, -1, scriptFont, &textBox);

	opened = -200;
}

SDL_Rect TreasureChest::getBox()
{
	return box;
}

void TreasureChest::setState(states state)
{
	this->state = state;
	if (state == OPEN)
	{
		Mix_PlayChannel(-1, chest, 0);
		opened = frame;
		switch (type)
		{
		case NINJA_STARS:
			Player.addNinjaStars(5);
			break;
		case HEALTH_POTION:
			Player.addHealth(15);
			break;
		case MANA_POTION:
			Player.addMana(15);
			break;
		}
	}
}

void TreasureChest::update()
{
	if (checkCollision(Player.getBox(), box) && Player.checkAction() && state == CLOSED)
	{
		setState(OPEN);
	}
}

void TreasureChest::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_Rect text_destination = { textBox.x - Camera.x, textBox.y - Camera.y, textBox.w, textBox.h };

	if (state == OPEN)
		SDL_RenderCopy(renderer, open_texture, NULL, &destination);
	else
		SDL_RenderCopy(renderer, closed_texture, NULL, &destination);

	if (frame - opened < 120)
		SDL_RenderCopy(renderer, text_texture, NULL, &text_destination);
}
#pragma endregion TreasureChest Class

//Treasure Chest Manager Class and Functions
#pragma region TreasureChestManager
class TreasureChestManager
{
private:
	std::vector<TreasureChest> treasure_chests;
	SDL_Texture* open;
	SDL_Texture* closed;

public:
	void load();
	void addChest(int x, int y, TreasureChest::types type);
	void update();
	void draw();
	void deleteAll();
};

void TreasureChestManager::load()
{
	open = loadImage("Media/treasure_open.png");
	closed = loadImage("Media/treasure_closed.png");
}

void TreasureChestManager::addChest(int x, int y, TreasureChest::types type)
{
	std::string STRING;
	switch (type)
	{
	case TreasureChest::NINJA_STARS:
		STRING = "You found some ninja stars!";
		break;
	case TreasureChest::HEALTH_POTION:
		STRING = "You found some health!";
		break;
	case TreasureChest::MANA_POTION:
		STRING = "You found some  mana!";
		break;
	}

	treasure_chests.push_back(TreasureChest(x, y, type, STRING, open, closed));
}

void TreasureChestManager::update()
{
	for (int i = 0; i < treasure_chests.size(); i++)
	{
		if (checkCollision(Camera, treasure_chests.at(i).getBox()))
			treasure_chests.at(i).update();
	}
}

void TreasureChestManager::draw()
{
	for (int i = 0; i < treasure_chests.size(); i++)
	{
		if (checkCollision(Camera, treasure_chests.at(i).getBox()))
			treasure_chests.at(i).draw();
	}
}

void TreasureChestManager::deleteAll()
{
	for (int i = 0; i < treasure_chests.size(); i++)
	{
		treasure_chests.erase(treasure_chests.begin() + i);
	}
}
#pragma endregion TreasureChestManager Class

//********** THE TREASURE CHEST MANAGER *************
TreasureChestManager TreasureChestMang;
#pragma endregion Treasure Chests

//**************************************************** END TREASURE CHESTS *************************************









//***************************************** DOORWAYS ***********************************************

#pragma region Door
class Door
{
private:
	SDL_Texture* texture;
	SDL_Rect box;
	SDL_Rect clip;
	level_type level;

public:
	Door(int x, int y, SDL_Texture* texture, level_type level);
	SDL_Rect getBox();
	void update();
	void draw();
};

Door::Door(int x, int y, SDL_Texture* texture, level_type level)
{
	SDL_Rect temp = { 0, 512, 128, 128 };
	clip = temp;

	box.x = x - 88;
	box.y = y - 85;
	box.w = 128;
	box.h = 128;

	this->texture = texture;
	this->level = level;
}

SDL_Rect Door::getBox()
{
	return box;
}

void Door::update()
{
	if (checkCollision(box, Player.getBox()) && Player.checkAction())
	{
		Mix_PlayChannel(-1, door, 0);
		current_level = level;
	}
}

void Door::draw()
{
	SDL_Rect destination = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
	SDL_RenderCopy(renderer, texture, &clip, &destination);
}
#pragma endregion Door Class

#pragma region DoorManager
class DoorManager
{
private:
	std::vector<Door> doors;
	SDL_Texture* texture;

public:
	void load();
	void addDoor(int x, int y);
	void update();
	void draw();
	void deleteAll();
};

void DoorManager::load()
{
	texture = loadImage("Media/Doorway.png");
}

void DoorManager::addDoor(int x, int y)
{
	doors.push_back(Door(x, y, texture, LEVEL_ONE));
}

void DoorManager::update()
{
	for (int i = 0; i < doors.size(); i++)
		if (checkCollision(doors.at(i).getBox(), Camera))
			doors.at(i).update();
}

void DoorManager::draw()
{
	for (int i = 0; i < doors.size(); i++)
		if (checkCollision(doors.at(i).getBox(), Camera))
			doors.at(i).draw();
}

void DoorManager::deleteAll()
{
	for (int i = 0; i < doors.size(); i++)
	{
		doors.erase(doors.begin() + i);
	}
}
#pragma endregion DoorManager Class

DoorManager DoorMang;

//*************************************** END DOORWAYS **********************************************








//*************************************************** LEVEL MANAGER ****************************************

//Level Manager Class and Functions
#pragma region LevelManager
class LevelManager
{
public:
	typedef enum { NE1, NW1, SE1, SW1, NE2, NW2, SE2, SW2, NE3, NW3, SE3, SW3, NE4, NW4, SE4, SW4, NE5, NW5, SE5, SW5, //Walls
		WE1, WW1, WE2, WW2, //Walkways
		FN1, FW1, FE1, FS1, FN2, FW2, FE2, FS2, FN3, FW3, FE3, FS3, FN4, FW4, FE4, FS4, FN5, FW5, FE5, FS5,//Fire Walkways
		IN1, IW1, IE1, IS1, IN2, IW2, IE2, IS2, IN3, IW3, IE3, IS3, IN4, IW4, IE4, IS4, IN5, IW5, IE5, IS5, // Ice Walkways
		AN1, AW1, AE1, AS1, AN2, AW2, AE2, AS2, AN3, AW3, AE3, AS3, AN4, AW4, AE4, AS4, AN5, AW5, AE5, AS5, // Air Walkways
		TN1, TS1, TN2, TS2, //Totems
		CES, FBG, FLA, GRA, NBG, ROT, //Cat's Eyes, Far Background, Flare, Grass, Nearbackground, Roots
		NON, //No texture

		MCH, //Main Character
		SOR, GOB, LIZ, //Enemies
		WIZ, ROB, //AIs
		TCN, TCH, TCM, //Treasure Chests
		DOR //Doorway
	} textures;

	void loadTextures();
	SDL_Texture* loadImage(const char *filename);
	void initializeLevels();
	void loadLevel(level_type level);
	void draw();

private:
	//Global
	int texel_height;
	int texel_width;
	SDL_Rect *background;

	//Level index
	level_type prevLevel;
	int current_height;
	int current_width;

	//Home Level
	int home_texelw;
	int home_texelh;
	int home_level_width;
	int home_level_height;
	int home_level_size;
	SDL_Texture *home_level_background[2];
	Tile *home_level[780];
	textures home_level_blueprint[780];

	//Level One
	int levelone_texelw;
	int levelone_texelh;
	int levelone_level_width;
	int levelone_level_height;
	int levelone_level_size;
	SDL_Texture *levelone_level_background[2];
	Tile *levelone_level[3540];
	textures levelone_level_blueprint[3540];

	//Main Menu
	int levelmenu_texelw;
	int levelmenu_texelh;
	int levelmenu_level_width;
	int levelmenu_level_height;
	int levelmenu_level_size;
	SDL_Texture *levelmenu_level_background[2];
	Tile *levelmenu_level[240];
	textures levelmenu_level_blueprint[240];

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

	SDL_Texture *fire_wall_ne1;
	SDL_Texture *fire_wall_nw1;
	SDL_Texture *fire_wall_se1;
	SDL_Texture *fire_wall_sw1;
	SDL_Texture *fire_wall_ne2;
	SDL_Texture *fire_wall_nw2;
	SDL_Texture *fire_wall_se2;
	SDL_Texture *fire_wall_sw2;
	SDL_Texture *fire_wall_ne3;
	SDL_Texture *fire_wall_nw3;
	SDL_Texture *fire_wall_se3;
	SDL_Texture *fire_wall_sw3;
	SDL_Texture *fire_wall_ne4;
	SDL_Texture *fire_wall_nw4;
	SDL_Texture *fire_wall_se4;
	SDL_Texture *fire_wall_sw4;
	SDL_Texture *fire_wall_ne5;
	SDL_Texture *fire_wall_nw5;
	SDL_Texture *fire_wall_se5;
	SDL_Texture *fire_wall_sw5;

	SDL_Texture *ice_wall_ne1;
	SDL_Texture *ice_wall_nw1;
	SDL_Texture *ice_wall_se1;
	SDL_Texture *ice_wall_sw1;
	SDL_Texture *ice_wall_ne2;
	SDL_Texture *ice_wall_nw2;
	SDL_Texture *ice_wall_se2;
	SDL_Texture *ice_wall_sw2;
	SDL_Texture *ice_wall_ne3;
	SDL_Texture *ice_wall_nw3;
	SDL_Texture *ice_wall_se3;
	SDL_Texture *ice_wall_sw3;
	SDL_Texture *ice_wall_ne4;
	SDL_Texture *ice_wall_nw4;
	SDL_Texture *ice_wall_se4;
	SDL_Texture *ice_wall_sw4;
	SDL_Texture *ice_wall_ne5;
	SDL_Texture *ice_wall_nw5;
	SDL_Texture *ice_wall_se5;
	SDL_Texture *ice_wall_sw5;

	SDL_Texture *air_wall_ne1;
	SDL_Texture *air_wall_nw1;
	SDL_Texture *air_wall_se1;
	SDL_Texture *air_wall_sw1;
	SDL_Texture *air_wall_ne2;
	SDL_Texture *air_wall_nw2;
	SDL_Texture *air_wall_se2;
	SDL_Texture *air_wall_sw2;
	SDL_Texture *air_wall_ne3;
	SDL_Texture *air_wall_nw3;
	SDL_Texture *air_wall_se3;
	SDL_Texture *air_wall_sw3;
	SDL_Texture *air_wall_ne4;
	SDL_Texture *air_wall_nw4;
	SDL_Texture *air_wall_se4;
	SDL_Texture *air_wall_sw4;
	SDL_Texture *air_wall_ne5;
	SDL_Texture *air_wall_nw5;
	SDL_Texture *air_wall_se5;
	SDL_Texture *air_wall_sw5;

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

	SDL_Texture *title_screen;

#pragma endregion Texture Declarations

	void buildLevel(textures blueprint[], Tile *level_boxes[], int height, int width, int tex_width, int tex_height);
	void LevelManager::loadLevelFromText(std::string filename, level_type level);
	LevelManager::textures LevelManager::stringToEnum(std::string enumString);
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

	fire_wall_ne1 = loadImage("Media/Walls/Wall 1 NE.png fire");
	fire_wall_nw1 = loadImage("Media/Walls/Wall 1 NW.png fire");
	fire_wall_se1 = loadImage("Media/Walls/Wall 1 SE.png fire");
	fire_wall_sw1 = loadImage("Media/Walls/Wall 1 SW.png fire");
	fire_wall_ne2 = loadImage("Media/Walls/Wall 2 NE.png fire");
	fire_wall_nw2 = loadImage("Media/Walls/Wall 2 NW.png fire");
	fire_wall_se2 = loadImage("Media/Walls/Wall 2 SE.png fire");
	fire_wall_sw2 = loadImage("Media/Walls/Wall 2 SW.png fire");
	fire_wall_ne3 = loadImage("Media/Walls/Wall 3 NE.png fire");
	fire_wall_nw3 = loadImage("Media/Walls/Wall 3 NW.png fire");
	fire_wall_se3 = loadImage("Media/Walls/Wall 3 SE.png fire");
	fire_wall_sw3 = loadImage("Media/Walls/Wall 3 SW.png fire");
	fire_wall_ne4 = loadImage("Media/Walls/Wall 4 NE.png fire");
	fire_wall_nw4 = loadImage("Media/Walls/Wall 4 NW.png fire");
	fire_wall_se4 = loadImage("Media/Walls/Wall 4 SE.png fire");
	fire_wall_sw4 = loadImage("Media/Walls/Wall 4 SW.png fire");
	fire_wall_ne5 = loadImage("Media/Walls/Wall 5 NE.png fire");
	fire_wall_nw5 = loadImage("Media/Walls/Wall 5 NW.png fire");
	fire_wall_se5 = loadImage("Media/Walls/Wall 5 SE.png fire");
	fire_wall_sw5 = loadImage("Media/Walls/Wall 5 SW.png fire");

	ice_wall_ne1 = loadImage("Media/Walls/Wall 1 NE.png ice");
	ice_wall_nw1 = loadImage("Media/Walls/Wall 1 NW.png ice");
	ice_wall_se1 = loadImage("Media/Walls/Wall 1 SE.png ice");
	ice_wall_sw1 = loadImage("Media/Walls/Wall 1 SW.png ice");
	ice_wall_ne2 = loadImage("Media/Walls/Wall 2 NE.png ice");
	ice_wall_nw2 = loadImage("Media/Walls/Wall 2 NW.png ice");
	ice_wall_se2 = loadImage("Media/Walls/Wall 2 SE.png ice");
	ice_wall_sw2 = loadImage("Media/Walls/Wall 2 SW.png ice");
	ice_wall_ne3 = loadImage("Media/Walls/Wall 3 NE.png ice");
	ice_wall_nw3 = loadImage("Media/Walls/Wall 3 NW.png ice");
	ice_wall_se3 = loadImage("Media/Walls/Wall 3 SE.png ice");
	ice_wall_sw3 = loadImage("Media/Walls/Wall 3 SW.png ice");
	ice_wall_ne4 = loadImage("Media/Walls/Wall 4 NE.png ice");
	ice_wall_nw4 = loadImage("Media/Walls/Wall 4 NW.png ice");
	ice_wall_se4 = loadImage("Media/Walls/Wall 4 SE.png ice");
	ice_wall_sw4 = loadImage("Media/Walls/Wall 4 SW.png ice");
	ice_wall_ne5 = loadImage("Media/Walls/Wall 5 NE.png ice");
	ice_wall_nw5 = loadImage("Media/Walls/Wall 5 NW.png ice");
	ice_wall_se5 = loadImage("Media/Walls/Wall 5 SE.png ice");
	ice_wall_sw5 = loadImage("Media/Walls/Wall 5 SW.png ice");

	air_wall_ne1 = loadImage("Media/Walls/Wall 1 NE.png air");
	air_wall_nw1 = loadImage("Media/Walls/Wall 1 NW.png air");
	air_wall_se1 = loadImage("Media/Walls/Wall 1 SE.png air");
	air_wall_sw1 = loadImage("Media/Walls/Wall 1 SW.png air");
	air_wall_ne2 = loadImage("Media/Walls/Wall 2 NE.png air");
	air_wall_nw2 = loadImage("Media/Walls/Wall 2 NW.png air");
	air_wall_se2 = loadImage("Media/Walls/Wall 2 SE.png air");
	air_wall_sw2 = loadImage("Media/Walls/Wall 2 SW.png air");
	air_wall_ne3 = loadImage("Media/Walls/Wall 3 NE.png air");
	air_wall_nw3 = loadImage("Media/Walls/Wall 3 NW.png air");
	air_wall_se3 = loadImage("Media/Walls/Wall 3 SE.png air");
	air_wall_sw3 = loadImage("Media/Walls/Wall 3 SW.png air");
	air_wall_ne4 = loadImage("Media/Walls/Wall 4 NE.png air");
	air_wall_nw4 = loadImage("Media/Walls/Wall 4 NW.png air");
	air_wall_se4 = loadImage("Media/Walls/Wall 4 SE.png air");
	air_wall_sw4 = loadImage("Media/Walls/Wall 4 SW.png air");
	air_wall_ne5 = loadImage("Media/Walls/Wall 5 NE.png air");
	air_wall_nw5 = loadImage("Media/Walls/Wall 5 NW.png air");
	air_wall_se5 = loadImage("Media/Walls/Wall 5 SE.png air");
	air_wall_sw5 = loadImage("Media/Walls/Wall 5 SW.png air");

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

	title_screen = loadImage("Media/mainmenu.png");

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
	loadLevelFromText("Media/Levels/levelhome.txt", HOME_LEVEL);

	//level one
	levelone_texelw = 40;
	levelone_texelh = 40;
	levelone_level_width = 295;
	levelone_level_height = 12;
	levelone_level_size = levelone_level_width*levelone_level_height;
	levelone_level_background[0] = far_background;
	levelone_level_background[1] = NULL;
	loadLevelFromText("Media/Levels/levelone.txt", LEVEL_ONE);

	//main menu
	levelmenu_texelw = 40;
	levelmenu_texelh = 40;
	levelmenu_level_width = 20;
	levelmenu_level_height = 12;
	levelmenu_level_size = levelmenu_level_width*levelmenu_level_height;
	levelmenu_level_background[0] = title_screen; 
	levelmenu_level_background[1] = NULL;
	loadLevelFromText("Media/Levels/levelmenu.txt", MENU_LEVEL);

}

void LevelManager::loadLevel(level_type level)
{
	prevLevel = level;
	current_level = level;

	EnemyMang.deleteAll();
	AIMang.deleteAll();
	TreasureChestMang.deleteAll();
	DoorMang.deleteAll();

	switch (current_level)
	{
	case HOME_LEVEL:
		buildLevel(home_level_blueprint, home_level, home_level_height, home_level_width, home_texelw, home_texelh);

		CurrentLevel = home_level;
		texel_width = home_texelw;
		texel_height = home_texelh;
		LevelWidth = home_level_width*home_texelw;
		LevelHeight = home_level_height*home_texelh;
		LevelSize = home_level_width*home_level_height;
		Background = home_level_background;
       	music = Mix_LoadMUS( "Media/Music/home_level.wav" );
		Mix_PlayMusic(music, -1);
		break;

	case LEVEL_ONE:
		buildLevel(levelone_level_blueprint, levelone_level, levelone_level_height, levelone_level_width, levelone_texelw, levelone_texelh);

		CurrentLevel = levelone_level;
		texel_width = levelone_texelw;
		texel_height = levelone_texelh;
		LevelWidth = levelone_level_width*home_texelw;
		LevelHeight = levelone_level_height*home_texelh;
		LevelSize = levelone_level_width*home_level_height;
		Background = levelone_level_background;
		music = Mix_LoadMUS( "Media/Music/level_two.wav" );
		Mix_PlayMusic(music, -1);
		break;

	case MENU_LEVEL:
		buildLevel(levelmenu_level_blueprint, levelmenu_level, levelmenu_level_height, levelmenu_level_width, levelmenu_texelw, levelmenu_texelh);

		CurrentLevel = levelmenu_level;
		texel_width = levelmenu_texelw;
		texel_height = levelmenu_texelh;
		LevelWidth = levelmenu_level_width*home_texelw;
		LevelHeight = levelmenu_level_height*home_texelh;
		LevelSize = levelmenu_level_width*home_level_height;
		Background = levelmenu_level_background;
		music = Mix_LoadMUS( "Media/Music/level_one.wav" );
		Mix_PlayMusic(music, -1);
		break;
	}

	Player.box.x = start_x;
	Player.box.y = start_y;
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
		case FN1:
			level = fire_wall_ne1;
			break;
		case FW1:
			level = fire_wall_nw1;
			break;
		case FE1:
			level = fire_wall_se1;
			break;
		case FS1:
			level = fire_wall_sw1;
			break;
		case FN2:
			level = fire_wall_ne2;
			break;
		case FW2:
			level = fire_wall_nw2;
			break;
		case FE2:
			level = fire_wall_se2;
			break;
		case FS2:
			level = fire_wall_sw2;
			break;
		case FN3:
			level = fire_wall_ne3;
			break;
		case FW3:
			level = fire_wall_nw3;
			break;
		case FE3:
			level = fire_wall_se3;
			break;
		case FS3:
			level = fire_wall_sw3;
			break;
		case FN4:
			level = fire_wall_ne4;
			break;
		case FW4:
			level = fire_wall_nw4;
			break;
		case FE4:
			level = fire_wall_se4;
			break;
		case FS4:
			level = fire_wall_sw4;
			break;
		case FN5:
			level = fire_wall_ne5;
			break;
		case FW5:
			level = fire_wall_nw5;
			break;
		case FE5:
			level = fire_wall_se5;
			break;
		case FS5:
			level = fire_wall_sw5;
			break;
		case IN1:
			level = ice_wall_ne1;
			break;
		case IW1:
			level = ice_wall_nw1;
			break;
		case IE1:
			level = ice_wall_se1;
			break;
		case IS1:
			level = ice_wall_sw1;
			break;
		case IN2:
			level = ice_wall_ne2;
			break;
		case IW2:
			level = ice_wall_nw2;
			break;
		case IE2:
			level = ice_wall_se2;
			break;
		case IS2:
			level = ice_wall_sw2;
			break;
		case IN3:
			level = ice_wall_ne3;
			break;
		case IW3:
			level = ice_wall_nw3;
			break;
		case IE3:
			level = ice_wall_se3;
			break;
		case IS3:
			level = ice_wall_sw3;
			break;
		case IN4:
			level = ice_wall_ne4;
			break;
		case IW4:
			level = ice_wall_nw4;
			break;
		case IE4:
			level = ice_wall_se4;
			break;
		case IS4:
			level = ice_wall_sw4;
			break;
		case IN5:
			level = ice_wall_ne5;
			break;
		case IW5:
			level = ice_wall_nw5;
			break;
		case IE5:
			level = ice_wall_se5;
			break;
		case IS5:
			level = ice_wall_sw5;
			break;
		case AN1:
			level = air_wall_ne1;
			break;
		case AW1:
			level = air_wall_nw1;
			break;
		case AE1:
			level = air_wall_se1;
			break;
		case AS1:
			level = air_wall_sw1;
			break;
		case AN2:
			level = air_wall_ne2;
			break;
		case AW2:
			level = air_wall_nw2;
			break;
		case AE2:
			level = air_wall_se2;
			break;
		case AS2:
			level = air_wall_sw2;
			break;
		case AN3:
			level = air_wall_ne3;
			break;
		case AW3:
			level = air_wall_nw3;
			break;
		case AE3:
			level = air_wall_se3;
			break;
		case AS3:
			level = air_wall_sw3;
			break;
		case AN4:
			level = air_wall_ne4;
			break;
		case AW4:
			level = air_wall_nw4;
			break;
		case AE4:
			level = air_wall_se4;
			break;
		case AS4:
			level = air_wall_sw4;
			break;
		case AN5:
			level = air_wall_ne5;
			break;
		case AW5:
			level = air_wall_nw5;
			break;
		case AE5:
			level = air_wall_se5;
			break;
		case AS5:
			level = air_wall_sw5;
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
			EnemyMang.addEnemy((i%width)*tex_width, (i/width)*tex_height, Enemy::SMALL_ORC);
			break;
		case GOB:
			level = NULL;
			EnemyMang.addEnemy((i%width)*tex_width, (i/width)*tex_height, Enemy::GRIZZOLAR_BEAR);
			break;
		case LIZ:
			level = NULL;
			EnemyMang.addEnemy((i%width)*tex_width, (i/width)*tex_height, Enemy::LIZARD);
			break;
		case WIZ:
			level = NULL;
			AIMang.addAI((i%width)*tex_width, (i/width)*tex_height, AI::WIZARD);
			break;
		case ROB:
			level = NULL;
			AIMang.addAI((i%width)*tex_width, (i/width)*tex_height, AI::ROBOT);
			break;
		case TCN:
			level = NULL;
			TreasureChestMang.addChest((i%width)*tex_width, (i/width)*tex_height, TreasureChest::NINJA_STARS);
			break;
		case TCH:
			level = NULL;
			TreasureChestMang.addChest((i%width)*tex_width, (i/width)*tex_height, TreasureChest::HEALTH_POTION);
			break;
		case TCM:
			level = NULL;
			TreasureChestMang.addChest((i%width)*tex_width, (i/width)*tex_height, TreasureChest::MANA_POTION);
			break;
		case DOR:
			level = NULL;
			DoorMang.addDoor((i%width)*tex_width, (i/width)*tex_height);
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
	for (int i = 0; i < 2; i++)
	{
		if (Background[i] == NULL)
			continue;
		SDL_RenderCopy(renderer, Background[i], NULL, background);
	}
	for (int i = 0; i < LevelWidth/40*LevelHeight/40; i++)
	{
		if (CurrentLevel[i] == NULL)
			continue;
		CurrentLevel[i]->draw(Camera, renderer);
	}

	if (prevLevel != current_level)
		loadLevel(current_level);
}

void LevelManager::loadLevelFromText(std::string filename, level_type level)
{
	char tempchar;
	std::string tempEnum = "";
	int numberOfTiles = 0;
	ifstream ifile(filename.c_str());
	textures newText;

	while (!ifile.eof()){
		tempchar = ifile.get();
		if (tempchar > 44 && tempchar < 123)
			tempEnum += tempchar;
		
		else{
			if (level == HOME_LEVEL)
				home_level_blueprint[numberOfTiles] = stringToEnum(tempEnum);
			else if (level == LEVEL_ONE)
				levelone_level_blueprint[numberOfTiles] = stringToEnum(tempEnum);
			else if (level == MENU_LEVEL)
				levelmenu_level_blueprint[numberOfTiles] = stringToEnum(tempEnum);
			tempEnum = "";
			numberOfTiles++;
			ifile.get();
		}
	}
	
}

LevelManager::textures LevelManager::stringToEnum(std::string enumString)
{
#pragma region Conversions
	if (enumString == "NE1")
		return NE1;
	else if (enumString == "NW1")
		return NW1;
	else if (enumString == "SE1")
		return SE1;
	else if (enumString == "SW1")
		return SW1;
	else if (enumString == "NE2")
		return NE2;
	else if (enumString == "NW2")
		return NW2;
	else if (enumString == "SE2")
		return SE2;
	else if (enumString == "SW2")
		return SW2;
	else if (enumString == "NE3")
		return NE3;
	else if (enumString == "NW3")
		return NW3;
	else if (enumString == "SE3")
		return SE3;
	else if (enumString == "SW3")
		return SW3;
	else if (enumString == "NE4")
		return NE4;
	else if (enumString == "NW4")
		return NW4;
	else if (enumString == "SE4")
		return SE4;
	else if (enumString == "SW4")
		return SW4;
	else if (enumString == "NE5")
		return NE5;
	else if (enumString == "NW5")
		return NW5;
	else if (enumString == "SE5")
		return SE5;
	else if (enumString == "SW5")
		return SW5;
	else if (enumString == "FN1")
		return FN1;
	else if (enumString == "FW1")
		return FW1;
	else if (enumString == "FE1")
		return FE1;
	else if (enumString == "FS1")
		return FS1;
	else if (enumString == "FN2")
		return FN2;
	else if (enumString == "FW2")
		return FW2;
	else if (enumString == "FE2")
		return FE2;
	else if (enumString == "FS2")
		return FS2;
	else if (enumString == "FN3")
		return FN3;
	else if (enumString == "FW3")
		return FW3;
	else if (enumString == "FE3")
		return FE3;
	else if (enumString == "FS3")
		return FS3;
	else if (enumString == "FN4")
		return FN4;
	else if (enumString == "FW4")
		return FW4;
	else if (enumString == "FE4")
		return FE4;
	else if (enumString == "FS4")
		return FS4;
	else if (enumString == "FN5")
		return FN5;
	else if (enumString == "FW5")
		return FW5;
	else if (enumString == "FE5")
		return FE5;
	else if (enumString == "FS5")
		return FS5;
	else if (enumString == "IN1")
		return IN1;
	else if (enumString == "IW1")
		return IW1;
	else if (enumString == "IE1")
		return IE1;
	else if (enumString == "IS1")
		return IS1;
	else if (enumString == "IN2")
		return IN2;
	else if (enumString == "IW2")
		return IW2;
	else if (enumString == "IE2")
		return IE2;
	else if (enumString == "IS2")
		return IS2;
	else if (enumString == "IN3")
		return IN3;
	else if (enumString == "IW3")
		return IW3;
	else if (enumString == "IE3")
		return IE3;
	else if (enumString == "IS3")
		return IS3;
	else if (enumString == "IN4")
		return IN4;
	else if (enumString == "IW4")
		return IW4;
	else if (enumString == "IE4")
		return IE4;
	else if (enumString == "IS4")
		return IS4;
	else if (enumString == "IN5")
		return IN5;
	else if (enumString == "IW5")
		return IW5;
	else if (enumString == "IE5")
		return IE5;
	else if (enumString == "IS5")
		return IS5;
	else if (enumString == "AN1")
		return AN1;
	else if (enumString == "AW1")
		return AW1;
	else if (enumString == "AE1")
		return AE1;
	else if (enumString == "AS1")
		return AS1;
	else if (enumString == "AN2")
		return AN2;
	else if (enumString == "AW2")
		return AW2;
	else if (enumString == "AE2")
		return AE2;
	else if (enumString == "AS2")
		return AS2;
	else if (enumString == "AN3")
		return AN3;
	else if (enumString == "AW3")
		return AW3;
	else if (enumString == "AE3")
		return AE3;
	else if (enumString == "AS3")
		return AS3;
	else if (enumString == "AN4")
		return AN4;
	else if (enumString == "AW4")
		return AW4;
	else if (enumString == "AE4")
		return AE4;
	else if (enumString == "AS4")
		return AS4;
	else if (enumString == "AN5")
		return AN5;
	else if (enumString == "AW5")
		return AW5;
	else if (enumString == "AE5")
		return AE5;
	else if (enumString == "AS5")
		return AS5;
	else if (enumString == "WE1")
		return WE1;
	else if (enumString == "WW1")
		return WW1;
	else if (enumString == "WE2")
		return WE2;
	else if (enumString == "WW2")
		return WW2;
	else if (enumString == "TN1")
		return TN1;
	else if (enumString == "TS1")
		return TS1;
	else if (enumString == "TN2")
		return TN2;
	else if (enumString == "TS2")
		return TS2;
	else if (enumString == "GRA")
		return GRA;
	else if (enumString == "ROT")
		return ROT;
	else if (enumString == "NON")
		return NON;
	else if (enumString == "MCH")
		return MCH;
	else if (enumString == "SOR")
		return SOR;
	else if (enumString == "GOB")
		return GOB;
	else if (enumString == "LIZ")
		return LIZ;
	else if (enumString == "WIZ")
		return WIZ;
	else if (enumString == "ROB")
		return ROB;
	else if (enumString == "TCN")
		return TCN;
	else if (enumString == "TCH")
		return TCH;
	else if (enumString == "TCM")
		return TCM;
	else if (enumString == "DOR")
		return DOR;
	else
		return NON;
#pragma endregion Conversions
}
#pragma endregion LevelManager Class

//******** THE LEVEL MANAGER ********
LevelManager LevelMang;

//************************************************** END LEVEL MANAGER ***********************************




#pragma region Main
//Initializes SDL, the window, renderer, and screen (Read LazyFoo)
bool init()
{
	if (SDL_Init( SDL_INIT_EVERYTHING ) == -1)
		return false;
	
	if (TTF_Init() == -1)
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

	gFont = TTF_OpenFont( "Media/Fonts/alagard.ttf", 30 );
	if( gFont == NULL )
	{
		printf( "Failed to load alagard font! SDL_ttf Error: %s\n", TTF_GetError() );
	}

	scriptFont = TTF_OpenFont( "Media/Fonts/romulus.ttf", 18 );
	if( scriptFont == NULL )
	{
		printf( "Failed to load lazy font! SDL_ttf Error: %s\n", TTF_GetError() );
	}

	if (window == NULL)
		return false;
	if(Mix_OpenAudio( 22050, MIX_DEFAULT_FORMAT, 2, 4096) == -1 )
	    return false;

	return true;
}

//Loads all the files
bool loadFiles()
{
	EnemyMang.load();
	TreasureChestMang.load();
	DoorMang.load();
	AIMang.load();

	LevelMang.loadTextures();
	LevelMang.initializeLevels();

	//Initialize to home level
	LevelMang.loadLevel(MENU_LEVEL);

	Player.load();
	Player.init(start_x, start_y);

	//load sounds
    door = Mix_LoadWAV( "Media/Music/door.wav" );
    ogre = Mix_LoadWAV( "Media/Music/ogre.wav" );
    swing = Mix_LoadWAV( "Media/Music/swing.wav" );
    clank = Mix_LoadWAV( "Media/Music/clank.wav" );
	chest = Mix_LoadWAV("Media/Music/treasure.wav" );
	fire = Mix_LoadWAV("Media/Music/fire.wav");
	burr = Mix_LoadWAV("Media/Music/burr.wav");
	lizard = Mix_LoadWAV("Media/Music/burr.wav");
	ice = Mix_LoadWAV("Media/Music/ice.wav");
	air = Mix_LoadWAV("Media/Music/air.wav");
	earth = Mix_LoadWAV("Media/Music/earth.wav");

	textArea = loadImage("Media/Fonts/textbox.png");

	ExitButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 60);
	SaveButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
	ResumeButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 - 70);
	PauseMenu.addButton(ExitButton);
	PauseMenu.addButton(SaveButton);
	PauseMenu.addButton(ResumeButton);
	
	NewButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 +70);
	LoadButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 120);
	QuitButton.loadButton(SCREEN_WIDTH/2, SCREEN_HEIGHT/2 + 170);
	MainMenu.addButton(NewButton);
	MainMenu.addButton(LoadButton);
	MainMenu.addButton(QuitButton);

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
	Player.movingCollision();
}

void update()
{
	collisionManager();

	Player.move();

	EnemyMang.update();

	TreasureChestMang.update();

	AIMang.update();

	DoorMang.update();

	setCamera(Player.getBox(), LevelWidth, LevelHeight);

	//Reset the player's actionCheck
	Player.resetActionCheck();
}

//Draws the screen and any additional things *** MAKE SURE TO PUT ANY DRAWING AFTER THE FIRST TWO LINES
void draw()
{
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, screen, NULL, NULL);

	LevelMang.draw();
	DoorMang.draw();
	TreasureChestMang.draw();
	AIMang.draw();
	Player.show(Camera.x, Camera.y);
	EnemyMang.draw();

	//Health is always last
	Player.drawStats();

	if (GAME_STATE == PAUSE_MENU)
		PauseMenu.draw();

	else if (GAME_STATE == MAIN_MENU)
		MainMenu.draw();

	SDL_RenderPresent(renderer);
}
#pragma endregion Main Functions

int main( int argc, char* args[] )
{
	//FPS
	frame = 0;

	//Initialize everything
	if (init() == false)
		return 1;

	//Load all files
	if (loadFiles() == false)
		return 1;

	//Main game loop
	while (!quit)
	{
		fps.start();
		while (SDL_PollEvent(&event))
		{
			switch (GAME_STATE)
			{
			case PLAYING:
				Player.handleInput(event);
				break;
			case AI_SPEAKING:
				SpeakingAI->handleInput(event);
				break;
			case PAUSE_MENU:
				PauseMenu.update(event);
				break;
			case MAIN_MENU:
				MainMenu.update(event);
				break;
			}

			//Quit options
			if (event.type == SDL_KEYDOWN)
			{
				if (event.key.keysym.sym == SDLK_ESCAPE)
					GAME_STATE = PAUSE_MENU;
			}
			if (event.type == SDL_QUIT) //X's out the window
				quit = true;
		}

		if (GAME_STATE == PLAYING)
			update();
		draw(); 

		//Regulates frame rate
		frame++;

		if (fps.getTicks() < 1000/FPS)
			SDL_Delay((1000/FPS) - fps.getTicks());

	}
    return 0;    
}