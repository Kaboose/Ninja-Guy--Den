#include "Tile.h"

Tile::Tile(int x, int y, int width, int height, SDL_Texture* tex, bool collide)
{
	SDL_Rect temp = { x, y, width, height };
	box = temp;
	texture = tex;
	solid = collide;
}

SDL_Rect Tile::getBox()
{
	return box;
}

bool Tile::collidable()
{
	return solid;
}

bool Tile::checkCollision( SDL_Rect a, SDL_Rect b )
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

void Tile::draw(SDL_Rect Camera, SDL_Renderer *renderer)
{
	if (checkCollision(box, Camera))
	{
		SDL_Rect clip = { box.x - Camera.x, box.y - Camera.y, box.w, box.h };
		SDL_RenderCopy(renderer, texture, NULL, &clip);
	}
}