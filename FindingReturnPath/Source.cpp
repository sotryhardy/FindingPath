#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include <Windows.h>
#include <string.h>

typedef unsigned char uchar;
typedef unsigned int uint;
typedef uchar mapType;			//if i will want to change the type for the map i will change it here

uint SCREEN_WIDTH;
uint SCREEN_HIGHT;

const int WIDTH = 15;
const int HEIGHT = 11;

enum WorldIndexes
{
	field = 1,
	//road...
	//swamp...
	obstacle = 255,
};

struct Vec2i
{
	int X;
	int Y;
};

struct Vec2f
{
	float X;
	float Y;
};

struct Vec4c
{
	uchar R;
	uchar G;
	uchar B;
	uchar A;
};

struct Image
{
	void Init(const char* image_name, Vec2i texSize, SDL_Renderer* renderer);
	void Destroy();

	void Render(SDL_Renderer* renderer, Vec2f position);
	void CreateRGBRect(SDL_Renderer* renderer, Vec4c colors, Vec2i tex_size);

	SDL_Texture* texture;
	Vec2i size;
};

void Image::Init(const char* image_name, Vec2i texSize, SDL_Renderer* renderer)
{
	SDL_Surface* surface = IMG_Load(image_name);
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", image_name, IMG_GetError());
		return ;
	}


	texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return ;
	}

	size = texSize;
	
	SDL_FreeSurface(surface);
}

void Image::Destroy()
{
	SDL_DestroyTexture(texture);
}

void Image::Render(SDL_Renderer* renderer, Vec2f position)
{
	SDL_Rect rect;
	rect.x = (int)position.X * size.X;
	rect.y = (int)position.Y * size.Y;
	rect.w = size.X;
	rect.h = size.Y;

	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);
}

void Image::CreateRGBRect(SDL_Renderer* renderer, Vec4c colors, Vec2i tex_size)
{
	SDL_Surface* surface;
	size = tex_size;
	SDL_Rect rect = { 0, 0, tex_size.X, tex_size.Y };
	surface = SDL_CreateRGBSurface(0, tex_size.X, tex_size.Y, 32, 0, 0, 0, 0);
	SDL_FillRect(surface, &rect, SDL_MapRGB(surface->format, colors.R, colors.G, colors.B));
	texture = SDL_CreateTextureFromSurface(renderer, surface);
	SDL_FreeSurface(surface);
}



struct Character
{
	void Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer);
	void Destroy();

	void Render(SDL_Renderer* renderer);

	Image texture;
	Vec2f position;
};

void Character::Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer)
{
	texture.Init(fileName, texSize, renderer);
	position = startPosition;
}

void Character::Render(SDL_Renderer* renderer)
{
	texture.Render(renderer, position);
}

void Character::Destroy()
{
	texture.Destroy();
}

struct ListNode
{
	ListNode* another_node;
	Vec2i position;
};

struct Stack
{
	ListNode* LastNode = nullptr;
	void AddElement(Vec2i nodePosition);
	void DeleteLastNode();
	void Clear();
};

void Stack::AddElement(Vec2i nodePosition)
{
	ListNode* NewNode = (ListNode*)malloc(sizeof(ListNode));
	NewNode->another_node = LastNode;
	NewNode->position = nodePosition;
	LastNode = NewNode;
}

void Stack::DeleteLastNode()
{
	if (LastNode)
	{
		ListNode* another_node = LastNode->another_node;
		free(LastNode);
		LastNode = another_node;
	}
}

void Stack::Clear()
{
	while (LastNode)
	{
		DeleteLastNode();
	}
}

struct Queue
{
	void AddNode(Vec2i nodePosition);
	bool IsEmpty();
	void DeleteFirstNode();
	void Clear();

	ListNode* FirstNode = nullptr;
};

void Queue::AddNode(Vec2i nodePosition)
{
	ListNode** last_node = &FirstNode;
	while (*last_node)
	{
		last_node = &(*last_node)->another_node;
	}
	*last_node = (ListNode*)malloc(sizeof(ListNode));
	(*last_node)->another_node = nullptr;
	(*last_node)->position = nodePosition;
}
void Queue::DeleteFirstNode()
{
	ListNode* next_node = FirstNode->another_node;
	free(FirstNode);
	FirstNode = next_node;

}
void Queue::Clear()
{
	while (FirstNode)
	{
		DeleteFirstNode();
	}
}


bool Queue::IsEmpty()
{
	return !FirstNode;
}

int SDL_StartInit(SDL_Renderer** renderer, SDL_Window** window)
{
	SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	SCREEN_HIGHT = GetSystemMetrics(SM_CYSCREEN);
	SDL_SetMainReady();
	int result = 0;
	result = SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
	if (result)
	{
		printf("Can't initialize SDL. Error: %s", SDL_GetError());
		return -1;
	}

	result = IMG_Init(IMG_INIT_PNG);
	if (!(result & IMG_INIT_PNG))
	{
		printf("Can't initialize SDL image. Error: %s", SDL_GetError());
		return -1;
	}

	*window = SDL_CreateWindow("FirstSDL",
		0, 0,
		SCREEN_WIDTH, SCREEN_HIGHT,
		SDL_WINDOW_SHOWN);

	if (!*window)
		return -1;


	*renderer = SDL_CreateRenderer(*window, -1, SDL_RENDERER_ACCELERATED);
	if (!*renderer)
		return -1;

	return 0;
}

void SumString(char* string, const char* value)
{
	strcpy(string + strlen(string), value);
}

void DrawMap(mapType** grid)
{
	char map[120 * 90] = "";
	for (int i = 0; i < HEIGHT; i++)
	{
		for (int j = 0; j < WIDTH; j++)
		{
			char buf[10] = "";
			sprintf(buf, "%i\t", grid[i][j]);
			SumString(map, buf);
		}
		SumString(map,"\n\n\n");
	}
	SumString(map,"\n\n\n");
	printf("%s", map);
}

Queue FindPath(Vec2i start_position, Vec2i end_position, mapType** battlefield)
{
	//https://nrsyed.com/2017/12/30/animating-the-grassfire-path-planning-algorithm/
	// Algorytn is based on finnding path starting from the end to start 
	Queue search_queue ,path;
	search_queue.AddNode(end_position);
	if (start_position.X == end_position.X && start_position.Y == end_position.Y || battlefield[end_position.Y][end_position.X] == 255)
	{
		return path;
	}
	mapType** grid = (mapType**)malloc(sizeof(mapType*) * HEIGHT);	// copy original battlefield 
	for (int i = 0; i < HEIGHT; i++)
	{
		grid[i] = (mapType*)malloc(sizeof(uchar) * WIDTH);
		for (int j = 0; j < WIDTH; j++)
		{
			grid[i][j] = battlefield[i][j];
		}
	}

	grid[end_position.Y][end_position.X] += 1;			//End point designation

	while (!search_queue.IsEmpty() && path.IsEmpty()) 
	{
		const Vec2i current_pos = search_queue.FirstNode->position;
		search_queue.DeleteFirstNode();

		uchar next_value = grid[current_pos.Y][current_pos.X] + 1;

		for (int i = -1; i <= 1; i++)		//cheking neighbour points of current point 
			for (int j = -1; j <= 1; j++)
				if (i * j == 0 && i != j)
				{
					Vec2i neighbour = { current_pos.X + i,  current_pos.Y + j};
					if (neighbour.X == start_position.X && neighbour.Y == start_position.Y)
					{
						path.AddNode(neighbour);
						grid[neighbour.Y][neighbour.X] = next_value;
					}
					else if (neighbour.X >= 0 && neighbour.Y >= 0 && neighbour.X < WIDTH && neighbour.Y < HEIGHT)
					{
						if (grid[neighbour.Y][neighbour.X] != obstacle && grid[neighbour.Y][neighbour.X] == battlefield[neighbour.Y][neighbour.X])	// if neighbour is not obstacle and was not edited by previouses iterations 
						{
							search_queue.AddNode(neighbour); // add point to check-list 
							grid[neighbour.Y][neighbour.X] = next_value;
						}
					}
				}
	}

	search_queue.Clear(); 
	DrawMap(grid);			//TODO: on assection-based for debug configuration
	if (path.FirstNode)
	{
		Vec2i current_pos = path.FirstNode->position;
		int next_value = grid[current_pos.Y][current_pos.X] - 1;
		while (next_value > grid[end_position.Y][end_position.X])
		{
			for (int i = -1; i <= 1; i++)
				for (int j = -1; j <= 1; j++)
					if (i * j == 0 && i != j)
					{
						Vec2i neighbour{ current_pos.X + i, current_pos.Y + j};
						if (neighbour.X >= 0 && neighbour.Y >= 0 && neighbour.X < WIDTH && neighbour.Y < HEIGHT)
						{
							if (grid[neighbour.Y][neighbour.X] == next_value)
							{
								path.AddNode(neighbour);
								current_pos = neighbour;
								next_value--;
							}
						}
					}
		}
		path.AddNode(end_position);
	}
	for (int i = 0; i < HEIGHT; i++)
	{
		free(grid[i]);
	}
	free(grid);
	return path;
}

int main(int argc, char* argv[])
{
	SDL_Renderer* renderer;						//creating pointers in this locality zone
	SDL_Window* window;

	if (SDL_StartInit(&renderer, &window))		//Initialization of SDL
		return -1;								// If you need to include more you can do it in this function

	SDL_SetRenderDrawColor(renderer, 20, 13, 39, 255);		//fill empry with color

	Image BlackRect;
	Character Ship;

	Vec2i PointSize{ SCREEN_WIDTH / WIDTH , SCREEN_HIGHT / HEIGHT }; //size of 1 part of screen in pixels

	Ship.Init("image.png", PointSize, { 7.0f, 10.0f }, renderer);
	BlackRect.CreateRGBRect(renderer, { 0, 0, 0, 0 }, PointSize);	//I will use it for obstacle

	mapType** battlefield = (mapType**)malloc(sizeof(mapType*) * HEIGHT);
	for (int i = 0; i < HEIGHT; i++)
	{
		battlefield[i] = (uchar*)malloc(sizeof(uchar) * WIDTH);					//initialization of battlefield
		for (int j = 0; j < WIDTH; j++)
		{
			battlefield[i][j] = field;
		}
	}

	battlefield[2][7] = 255;
	battlefield[3][6] = 255;
	battlefield[4][7] = 255;			//initialization of obstacle's positions 
	battlefield[3][8] = 255;
	for (int i = 1; i < WIDTH - 1; i++)
	{
		battlefield[HEIGHT / 2][i] = 255;
	}

	Queue Path;
	bool done = false;
	SDL_Event sdl_event;
	while (!done)				//start main cycle  for game
	{
		int lasttime = SDL_GetTicks();
		while (SDL_PollEvent(&sdl_event))
		{
			if (sdl_event.type == SDL_QUIT)
			{
				done = true;
			}
			else if (sdl_event.type == SDL_KEYDOWN)
			{
				switch (sdl_event.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					SDL_Event event;
					event.type = SDL_QUIT;
					event.quit.type = SDL_QUIT;
					event.quit.timestamp = SDL_GetTicks();
					SDL_PushEvent(&event);
					break;
				default:
					break;
				}
			}
			else if (sdl_event.type == SDL_MOUSEBUTTONDOWN)	
			{
				// When LBM was clicked i start finding path
				switch (sdl_event.button.button)
				{
				case SDL_BUTTON_LEFT:
				{
					Vec2i screen_mouse_pos, world_mouse_pos;
					SDL_GetMouseState(&screen_mouse_pos.X, &screen_mouse_pos.Y);
					world_mouse_pos = { screen_mouse_pos.X / PointSize.X, screen_mouse_pos.Y / PointSize.Y };
					Path.Clear();
					int last = SDL_GetTicks();
					Path = FindPath({ (int)Ship.position.X, (int)Ship.position.Y }, world_mouse_pos, battlefield);	//get the path from function
					printf("\n%i", SDL_GetTicks() - last);
					break;
				}
				default:
					break;
				}
			}
		}

		SDL_RenderClear(renderer);

		if (Path.FirstNode)
		{
			Ship.position = { (float)Path.FirstNode->position.X, (float)Path.FirstNode->position.Y };	//move character in the right point
			Path.DeleteFirstNode();
			Sleep(100);			//!!!!!			//TODO: on mover-based
		}

		for (int i = 0; i < HEIGHT; i++)
		{
			for (int j = 0; j < WIDTH; j++)
			{
				if (battlefield[i][j] == 255)
				{																			// } rendering obstacles 
					BlackRect.Render(renderer, { (float)j, (float)i });
				}
			}
		}

		Ship.Render(renderer);

		SDL_RenderPresent(renderer);

	}

	for (int i = 0; i < HEIGHT; i++)
	{
		free(battlefield[i]);
	}

	free(battlefield);
	Path.Clear();
	BlackRect.Destroy();
	Ship.Destroy();

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
