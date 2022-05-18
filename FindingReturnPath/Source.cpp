#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include <Windows.h>
#include <string.h>
#include <time.h>

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
	//hm... i cant use float here... TODO
	obstacle = 255,
};

enum Commands
{
	Neutral,
	Red,
	Blue
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

struct ListNode
{
	ListNode* another_node;
	Vec2i position;
};

struct Queue
{
	void AddNode(Vec2i nodePosition);
	bool IsEmpty();
	void DeleteFirstNode();
	void Clear();

	ListNode* FirstNode = nullptr;
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
	rect.x = position.X * size.X;
	rect.y = position.Y * size.Y;
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
	void Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer, float ch_speed, Commands ch_side);
	void Destroy();

	void Render(SDL_Renderer* renderer);
	void MoveInit(Queue _path);
	void Move(float Deltatime);
	bool IsMove();

	Image texture;
	Vec2f position;
	Queue path;
	Commands side;
	float speed;
	bool do_get_position;
};

void Character::Init(const char* fileName, Vec2i texSize, Vec2f startPosition, SDL_Renderer* renderer, float ch_speed, Commands ch_side = Neutral)
{
	texture.Init(fileName, texSize, renderer);
	position = startPosition;
	side = ch_side;
	speed = ch_speed;
	do_get_position = true;
}

void Character::Destroy()
{
	texture.Destroy();
}

void Character::Render(SDL_Renderer* renderer)
{
	texture.Render(renderer, position);
}

void Character::MoveInit(Queue _path)
{
	path.Clear();
	path = _path;
}

void Character::Move(float Deltatime)
{
	if (!path.IsEmpty())
	{
		static Vec2i goal, direction;
		if (do_get_position)
		{
			goal = path.FirstNode->position;
			direction = { goal.X - (int)position.X, goal.Y - (int)position.Y };
			do_get_position = false;
		}
		position.X += direction.X * speed * Deltatime;
		position.Y += direction.Y * speed * Deltatime;
		if (fabs(position.X - goal.X) < speed * Deltatime  && fabs(position.Y - goal.Y) < speed * Deltatime )
		{
			position = { (float)goal.X, (float)goal.Y };
			do_get_position = true;
			path.DeleteFirstNode();
		}
	}
}

bool Character::IsMove()
{
	return !path.IsEmpty();
}

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


bool Queue::IsEmpty()
{
	return !FirstNode;
}

int SDL_StartInit(SDL_Renderer** renderer, SDL_Window** window)
{
	SCREEN_WIDTH = GetSystemMetrics(SM_CXSCREEN);
	SCREEN_HIGHT = GetSystemMetrics(SM_CYSCREEN);
	srand(time(NULL));
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

Queue FindPath(Vec2i start_position, Vec2i end_position, mapType** battlefield, Character* characters = nullptr, int amount = 0)
{
	//https://nrsyed.com/2017/12/30/animating-the-grassfire-path-planning-algorithm/
	// Algorytn is based on finnding path starting from the end to start 
	Queue search_queue ,path;
	search_queue.AddNode(end_position);

	mapType** grid = (mapType**)malloc(sizeof(mapType*) * HEIGHT);	// copy original battlefield 
	for (int i = 0; i < HEIGHT; i++)
	{
		grid[i] = (mapType*)malloc(sizeof(uchar) * WIDTH);
		for (int j = 0; j < WIDTH; j++)
		{
			grid[i][j] = battlefield[i][j];
		}
	}

	for(int i = 0; i < amount; i++)
	{
		Vec2f pos = characters[i].position;
		grid[(int)pos.Y][(int)pos.X] = obstacle;
	}

	if (grid[end_position.Y][end_position.X] == obstacle)
	{
		return path;
	}

	grid[end_position.Y][end_position.X] = battlefield[end_position.Y][end_position.X] + 1;			//End point designation

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

void CharactersInit(Character* arr, int amount, Vec2i PointSize, SDL_Renderer* renderer, float character_speed)
{
	int above_indent = 1;
	int size_indent = 1;
	for (int i = 0; i < amount; i++)
	{
		if (!(i % 2))
			arr[i].Init("image.png", PointSize, { (float)size_indent, (float)above_indent + i + 1 }, renderer, character_speed, Red);
		else
			arr[i].Init("image.png", PointSize, { (float)WIDTH - 1 - size_indent, (float)above_indent + i }, renderer, character_speed, Blue);
	}
}

int main(int argc, char* argv[])
{
	SDL_Renderer* renderer;						//creating pointers in this locality zone
	SDL_Window* window;
	bool LBM = false;
	bool PlayerPart = true;
	bool EnemyPart = false;

	if (SDL_StartInit(&renderer, &window))		//Initialization of SDL
		return -1;								// If you need to include more you can do it in this function

	SDL_SetRenderDrawColor(renderer, 20, 13, 39, 255);		//fill empty with color

	Image BlackRect;
	
	int CharactersAmount = 8; 
	int PlayerAmount = CharactersAmount / 2;
	int EnemyAmount = CharactersAmount / 2;
	Character* Characters = new Character[CharactersAmount];

	float character_speed = 10.0f;

	Vec2i PointSize{ SCREEN_WIDTH / WIDTH , SCREEN_HIGHT / HEIGHT }; //size of 1 part of screen in pixels

	CharactersInit(Characters, CharactersAmount, PointSize, renderer, character_speed);

	Character** PlayerCommand = new Character * [PlayerAmount];
	Character** EnemyCommand = new Character * [EnemyAmount];

	for (int i = 0; i < CharactersAmount; i++)
	{
		static int Pindex = 0, Eindex = 0;
		if (Characters[i].side == Red)
		{
			PlayerCommand[Pindex] = &Characters[i];
			Pindex++;
		}
		else
		{
			EnemyCommand[Eindex] = &Characters[i];
			Eindex++;
		}

	}

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

	battlefield[2][7] = obstacle;
	battlefield[3][6] = obstacle;
	battlefield[4][7] = obstacle;			//initialization of obstacle's positions 
	battlefield[3][8] = obstacle;
	for (int i = 1; i < WIDTH - 1; i++)
	{
		battlefield[HEIGHT / 2][i] = 255;
	}

	Queue Path;
	bool done = false;
	SDL_Event sdl_event;
	float Deltatime = 0.016;
	bool CharacterIsMove = false;
	int PartIndex = 0;
	int Pindex = 0;
	int Eindex = 0;
	Character* CharacterToMove= PlayerCommand[0];
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
					LBM = true;
					
					break;
				}
				default:
					break;
				}
			}
		}

		SDL_RenderClear(renderer);

		if (LBM && PlayerPart)
		{
			LBM = false;
			PlayerPart = false;
			CharacterIsMove = true;
			CharacterToMove = PlayerCommand[Pindex % PlayerAmount];
			Pindex++;
			Vec2i screen_mouse_pos, world_mouse_pos;
			SDL_GetMouseState(&screen_mouse_pos.X, &screen_mouse_pos.Y);
			world_mouse_pos = { screen_mouse_pos.X / PointSize.X, screen_mouse_pos.Y / PointSize.Y };
			int last = SDL_GetTicks();
			CharacterToMove->MoveInit(FindPath({ (int)CharacterToMove->position.X, (int)CharacterToMove->position.Y }, world_mouse_pos, battlefield, Characters, CharactersAmount));	//get the path from function
			printf("\n%i", SDL_GetTicks() - last);
		}

		if (EnemyPart)
		{
			EnemyPart = false;
			CharacterIsMove = true;
			Vec2i goal = { rand() % WIDTH, rand() % HEIGHT };
			Vec2f pos = EnemyCommand[Eindex % EnemyAmount]->position;
			Queue Path = FindPath({ (int)pos.X, (int)pos.Y }, goal, battlefield, Characters, CharactersAmount);
			while (Path.IsEmpty())
			{
				goal = { rand() % WIDTH, rand() % HEIGHT };
				Path = FindPath({ (int)pos.X, (int)pos.Y }, goal, battlefield, Characters, CharactersAmount);
			}
			CharacterToMove = EnemyCommand[Eindex % EnemyAmount];
			Eindex++;
			CharacterToMove->MoveInit(Path);

		}
		
		if (CharacterIsMove && !(CharacterToMove->IsMove()))
		{
			CharacterIsMove = false;
			PartIndex++;
			if (PartIndex % 2)
			{
				EnemyPart = true;
			}
			else
			{
				PlayerPart = true;
			}
		}
		
		CharacterToMove->Move(Deltatime);


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

		for(int i = 0; i < CharactersAmount; i++)
			Characters[i].Render(renderer);

		SDL_RenderPresent(renderer);
		int TotalTime = SDL_GetTicks();
		int SleepTime = Deltatime * 1000 - (TotalTime - lasttime);
		if (SleepTime > 0)
		{
			Sleep(SleepTime);
		}

	}

	for (int i = 0; i < HEIGHT; i++)
	{
		free(battlefield[i]);
	}



	free(battlefield);
	Path.Clear();
	BlackRect.Destroy();
	Characters[0].Destroy();
	delete[] Characters;
	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	return 0;
}
