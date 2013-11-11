#include "SDL.h"
#include "SDL_image.h"

class Tile
{
public:
	Tile(int x, int y, int width, int height, SDL_Texture *tex, bool collide);
	void draw(SDL_Rect Camera, SDL_Renderer *renderer);
	SDL_Rect getBox();
	bool collidable();
	bool checkCollision(SDL_Rect a, SDL_Rect b);

private:
	SDL_Rect box;
	SDL_Texture *texture;
	bool solid;
};