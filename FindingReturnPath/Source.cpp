#include <stdio.h>
#include <stdlib.h>
#include "SDL.h"
#include "SDL_image.h"
#include <Windows.h>

const int WIDTH = 15;
const int HEIGHT = 11;

struct Position
{
	unsigned char X;
	unsigned char Y;
};

struct Block
{
	SDL_Texture* texture;
	int tex_wigth;
	int tex_height;
	void Init(SDL_Texture* tex, int width, int height);
	void Rendering(SDL_Renderer* renderer, double x, double y);
	void Destroy();
};

void Block::Init(SDL_Texture* tex, int width, int height)
{
	texture = tex;
	tex_wigth = width;
	tex_height = height;
}

void Block::Rendering(SDL_Renderer* renderer, double x, double y)
{
	SDL_Rect rect;
	rect.x = (int)round(x * 1920/WIDTH); 
	rect.y = (int)round(y * 1080/HEIGHT);
	rect.w = (int)tex_wigth;
	rect.h = (int)tex_height;

	SDL_RenderCopyEx(renderer,
		texture,
		nullptr,
		&rect,
		0,
		nullptr,
		SDL_FLIP_NONE);
}

void Block::Destroy()
{
	SDL_DestroyTexture(texture);
}



struct Player
{
	Block block;
	double x;
	double y; 
	void Init(SDL_Texture* tex, int width, int height, int PositionX, int PositionY);
	void Rendering(SDL_Renderer* renderer);
	void Destroy();
};

void Player::Init(SDL_Texture* tex, int width, int height, int PositionX, int PositionY)
{
	block.Init(tex, width, height);
	x = PositionX;
	y = PositionY;
}

void Player::Rendering(SDL_Renderer* renderer)
{
	block.Rendering(renderer, x, y);
}

void Player::Destroy()
{
	SDL_DestroyTexture(block.texture);
}

struct List
{
	List* PastElement;
	unsigned char X;
	unsigned char Y;
};

struct Stack
{
	List* LastElement = nullptr;
	void AddElement(int x, int y)
	{
		List* NewElement = (List*)malloc(sizeof(List));
		NewElement->PastElement = LastElement;
		NewElement->X = x;
		NewElement->Y = y;
		LastElement = NewElement;
	}
	void DeleteLastElement()
	{
		if (LastElement)
		{
			List* PastElement = LastElement->PastElement;
			free(LastElement);
			LastElement = PastElement;
		}
		else
		{
			printf("You Delete non-existent element!");
			abort();
		}
	}

	void Clear()
	{
		while (LastElement)
		{
			DeleteLastElement();
		}
	}
};

int main(int argc, char* argv[])
{
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

	SDL_Window* window = SDL_CreateWindow("FirstSDL",
		0, 0,
		1920, 1080,
		SDL_WINDOW_SHOWN);

	if (!window)
		return -1;


	SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (!renderer)
		return -1;


	SDL_SetRenderDrawColor(renderer, 20, 13, 39, 255);

	char image_path[] = "image.png";
	SDL_Surface* surface = IMG_Load(image_path);
	if (!surface)
	{
		printf("Unable to load an image %s. Error: %s", image_path, IMG_GetError());
		return -1;
	}

	
	SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
	if (!texture)
	{
		printf("Unable to create a texture. Error: %s", SDL_GetError());
		return -1;
	}

	int tex_width = surface->w;
	int tex_height = surface->h;

	// Bye-bye the surface
	SDL_FreeSurface(surface);

	Block BlackBlock;
	Player Ship;
	SDL_Surface* s;

	int WidthPixels = 1920 / WIDTH;
	int HeightPixels = 1080 / HEIGHT;

	Position GoalPosition;

	s = SDL_CreateRGBSurface(0, WidthPixels, HeightPixels, 32, 0, 0, 0, 0);
	BlackBlock.Init(SDL_CreateTextureFromSurface(renderer, s), WidthPixels, HeightPixels);

	Ship.Init(texture, WidthPixels, HeightPixels, 7, 10);

	SDL_FreeSurface(s);


	unsigned char** arr = (unsigned char**)malloc(sizeof(unsigned char*) * HEIGHT);
	for (int i = 0; i < HEIGHT; i++)
	{
		arr[i] = (unsigned char*)malloc(sizeof(unsigned char) * WIDTH);
	}


	Stack Stack1, Stack2;
	Stack* CheckNow = &Stack1;
	Stack* CheckNext = &Stack2;
	Stack Path;

	
	bool finding = false;
	bool PathGenerate = false;
	bool GetTimeOfAlgorytm = false;

	bool done = false;
	SDL_Event sdl_event;
	while (!done)
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
			else if(sdl_event.type == SDL_MOUSEBUTTONDOWN)
			{
				switch (sdl_event.button.button)
				{
				case SDL_BUTTON_LEFT:
				{
					int x, y;
					SDL_GetMouseState(&x, &y);
					CheckNow->Clear();
					CheckNext->Clear();
					Path.Clear();
					CheckNow->AddElement(Ship.x, Ship.y);
					for (int i = 0; i < HEIGHT; i++)
					{
						for (int j = 0; j < WIDTH; j++)
						{
							arr[i][j] = 1;
						}
					}
					arr[(int)Ship.y][(int)Ship.x] = 2;
					arr[y / HeightPixels][x / WidthPixels] = 255;
					finding = true;
					GetTimeOfAlgorytm = true;
					break;
				}
				default:
					break;
				}
			}
		}

		// Clearing the screen
		SDL_RenderClear(renderer);

		for (int i = 1; i < WIDTH - 1; i++)
		{
			arr[HEIGHT / 2][i] = 0;
			BlackBlock.Rendering(renderer, i, HEIGHT / 2);
		
		}

		arr[2][7] = 0;
		arr[3][6] = 0;
		arr[4][7] = 0;
		arr[3][8] = 0;
		BlackBlock.Rendering(renderer, 7, 2);
		BlackBlock.Rendering(renderer, 6, 3);
		BlackBlock.Rendering(renderer, 7, 4);
		BlackBlock.Rendering(renderer, 8, 3);

		int StartTimer = SDL_GetTicks();
		while (finding)
		{
			unsigned char x = CheckNow->LastElement->X;
			unsigned char y = CheckNow->LastElement->Y;
			if (x - 1 >= 0 && (arr[y][x - 1] == 1 || arr[y][x - 1] == 255))		// LEFT POINT
			{
				if (arr[y][x - 1] != 255)
				{
					CheckNext->AddElement(x - 1, y);
				}
				else
				{
					Path.AddElement(x - 1, y);
					PathGenerate = true;
				}
				arr[y][x - 1] = arr[y][x] + 1;
			}
			
			
			if (y - 1 >= 0 && (arr[y - 1][x] == 1 || arr[y - 1][x] == 255))		//UP POINT
			{
				if (arr[y - 1][x] != 255)
				{	
					CheckNext->AddElement(x, y - 1);
				}
				else
				{
					Path.AddElement(x, y - 1);
					PathGenerate = true;
				}
				arr[y - 1][x] = arr[y][x] + 1;	
			}

			if (x + 1 < WIDTH && (arr[y][x + 1] == 1 || arr[y][x + 1] == 255))			//RIGTH POINT
			{
				
				if (arr[y][x + 1] != 255)
				{		
					CheckNext->AddElement(x + 1, y);
				}
				else
				{
					Path.AddElement(x + 1, y);
					PathGenerate = true;
				}
				arr[y][x + 1] = arr[y][x] + 1;
			}

			if (y + 1 < HEIGHT &&(arr[y + 1][x] == 1 || arr[y + 1][x] == 255))		//DOWN POINT
			{
				if (arr[y + 1][x] != 255)
				{
					CheckNext->AddElement(x, y + 1);
				}
				else
				{
					Path.AddElement(x, y + 1);
					PathGenerate = true;
				}
				arr[y + 1][x] = arr[y][x] + 1;
			}
			CheckNow->DeleteLastElement();
			if (!CheckNow->LastElement)
			{
				if (CheckNext->LastElement)
				{
					Stack* temp = CheckNow;
					CheckNow = CheckNext;
					CheckNext = temp;
				}
				else
				{
					finding = false;
					break;
				}
			}
			
		
			while (PathGenerate)
			{
				
				unsigned char x = Path.LastElement->X;
				unsigned char y = Path.LastElement->Y;
				if (arr[y][x] <= 3)
				{
					finding = false;
					PathGenerate = false;
					break;
				}
				if (x - 1 >= 0 && arr[y][x - 1] == arr[y][x] - 1)
				{
					Path.AddElement(x - 1, y);
				}
				else if (x + 1 < WIDTH && arr[y][x + 1] == arr[y][x] - 1)
				{
					Path.AddElement(x + 1, y);
				}
				else if (y - 1 >= 0 && arr[y - 1][x] == arr[y][x] - 1)
				{
					Path.AddElement(x, y - 1);
				}
				else if (y + 1 < HEIGHT && arr[y + 1][x] == arr[y][x] - 1)
				{
					Path.AddElement(x, y + 1);
				}
				
			}
		}
		if (GetTimeOfAlgorytm)
		{
			printf("time: %i\n", (SDL_GetTicks() - StartTimer));
			GetTimeOfAlgorytm = false;
		}
		
		if (Path.LastElement)
		{
			Ship.x = Path.LastElement->X;
			Ship.y = Path.LastElement->Y;
			Path.DeleteLastElement();
			Sleep(100);
		}

		Ship.Rendering(renderer);

		SDL_RenderPresent(renderer);

	}

	for (int i = 0; i < HEIGHT; i++)
	{
		free(arr[i]);
	}

	free(arr);

	BlackBlock.Destroy();
	Ship.Destroy();

	SDL_DestroyRenderer(renderer);

	SDL_DestroyWindow(window);
	IMG_Quit();
	SDL_Quit();

	// Done.
	return 0;
}
