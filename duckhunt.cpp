// Jason Thai
// Duck Hunt

#include <iostream>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>
#include "ppm.h"
#include <stdio.h>
#include <unistd.h> //for sleep function

//800, 600
#define WINDOW_WIDTH  800
#define WINDOW_HEIGHT 600

#define MAX_DUCKS 2

#define rnd() (float)rand() / (float)RAND_MAX

extern "C"
{
#include "fonts.h"
}

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

struct timespec timeStart, timeCurrent;
struct timespec timePause;
double movementCountdown = 0.0;
double timeSpan = 0.0;
const double oobillion = 1.0 / 1e9;
const double movementRate = 1.0 / 60.0;
double timeDiff(struct timespec *start, struct timespec *end)
{
	return (double)(end->tv_sec - start->tv_sec) + (double)(end->tv_nsec - start->tv_nsec) * oobillion;
}
void timeCopy(struct timespec *dest, struct timespec *source)
{
	memcpy(dest, source, sizeof(struct timespec));
}


//Structures
struct Vec {
	float x, y, z;
};

struct Shape {
	float width, height;
	float radius;
	Vec center;
};

//duck sprite
typedef double Arr[3];
struct Sprite {
	Arr pos;
	Arr vel;
};
//First Duck Sprite
Sprite duck_sprite;
Ppmimage *duckImage=NULL;
GLuint duckTexture;
GLuint duckSil;
//Second Duck Sprite
Sprite duck_sprite2;
Ppmimage *duckImage2=NULL;
GLuint duckTexture2;
GLuint duckSil2;
int show_duck = 0;
int silhouette = 1;
//Bullet Sprite
Sprite bullet_sprite;
Ppmimage *bulletImage=NULL;
GLuint bulletTexture;
//White duck Sprite
Sprite duckscore_sprite;
Ppmimage *duckscoreImage=NULL;
GLuint duckscoreTexture;
//Red duck sprite
Sprite duckscore_sprite2;
Ppmimage *duckscoreImage2=NULL;
GLuint duckscoreTexture2;

struct Duck
{
	Shape s;
	Vec velocity;
	struct timespec time;
	struct Duck *prev;
	struct Duck *next;
	//bool shot;
	Duck()
	{
		prev = NULL;
		next = NULL;
	}
};

struct Game {
	int bullets;
	Duck *duck;
	int n;
	float floor;
	int rounds;
	struct timespec duckTimer;
	//Shape box1[1];
	Shape box[4];
	int score;
	int duckCount;
	int duckShot;
	bool oneDuck, twoDuck;
	~Game()
	{
		delete [] duck;
	}
	Game()
	{
		duck = NULL;
		bullets = 0;
		n = 0;
		floor = WINDOW_HEIGHT / 5.0;
		rounds = 1;
		score = 0;
		duckCount = 0;
		oneDuck = false;
		twoDuck = false;
		/*for(int i = 0; i <= 2; i++)
		  {
		  box1[i].width = 45;
		  box1[i].height = 35;
		  box1[i].center.x = WINDOW_WIDTH - 675;
		  box1[i].center.y = WINDOW_HEIGHT - 550;
		//box1[i].center.x = WINDOW_HEIGHT - 550;
		//box1[i].center.y = WINDOW_WIDTH - 50;
		box1[i].center.z = 0;
		}*/
		box[0].width = 45;
		box[0].height = 35;
		box[0].center.x = (WINDOW_WIDTH / 10) - (WINDOW_WIDTH / 20);
		box[0].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
		box[0].center.z = 0;

		box[1].width = 100;
		box[1].height = 35;
		box[1].center.x = WINDOW_WIDTH / 4;
		box[1].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
		box[1].center.z = 0;

		box[2].width = 45;
		box[2].height = 35;
		box[2].center.x = (WINDOW_WIDTH / 2) + (WINDOW_WIDTH / 4);
		box[2].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.1);
		box[2].center.z = 0;

		box[3].width = 45;
		box[3].height = 35;
		box[3].center.x = (WINDOW_WIDTH / 10) - (WINDOW_WIDTH / 70);
		box[3].center.y = WINDOW_HEIGHT - (WINDOW_HEIGHT - floor) - (floor / 1.5);
		box[3].center.z = 0;
	}
};

//Function prototypes
void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_mouse(XEvent *e, Game *game);
int check_keys(XEvent *e, Game *game);
void movement(Game *game);
void render(Game *game);
void makeDuck(Game *game);
void deleteDuck(Game *game, Duck *duck);
void check_resize(XEvent *e);

Ppmimage *backgroundImage = NULL;
GLuint backgroundTexture;
int background = 1;
//Transparent background
Ppmimage *backgroundTransImage = NULL;
Ppmimage *gameoverbgImage = NULL;
GLuint backgroundTransTexture;
GLuint gameoverbgTexture;
int trees = 1;
bool gameover = false;

int main(void)
{
	int done=0;
	srand(time(NULL));
	initXWindows();
	init_opengl();

	clock_gettime(CLOCK_REALTIME, &timePause);
	clock_gettime(CLOCK_REALTIME, &timeStart);
	//declare game object
	Game game;

	//start animation
	while(!done) {
		while(XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_resize(&e);
			check_mouse(&e, &game);
			done = check_keys(&e, &game);
		}
		clock_gettime(CLOCK_REALTIME, &timeCurrent);
		timeSpan = timeDiff(&timeStart, &timeCurrent);
		timeCopy(&timeStart, &timeCurrent);
		movementCountdown += timeSpan;
		while(movementCountdown >= movementRate)
		{
			movement(&game);
			movementCountdown -= movementRate;
		}
		render(&game);
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void set_title(void)
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "Duck Hunt");
}

void cleanupXWindows(void) {
	//do not change
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void initXWindows(void) {
	//do not change
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w=WINDOW_WIDTH, h=WINDOW_HEIGHT;

	XSetWindowAttributes swa;

	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		std::cout << "\n\tcannot connect to X server\n" << std::endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if(vi == NULL) {
		std::cout << "\n\tno appropriate visual found\n" << std::endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	//XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
			InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height)
{
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);
	set_title();
}

unsigned char *buildAlphaData(Ppmimage *img) {
	// add 4th component to RGB stream...
	int a,b,c;
	unsigned char *newdata, *ptr;
	unsigned char *data = (unsigned char *)img->data;
	//newdata = (unsigned char *)malloc(img->width * img->height * 4);
	newdata = new unsigned char[img->width * img->height * 4];
	ptr = newdata;
	for (int i=0; i<img->width * img->height * 3; i+=3) {
		a = *(data+0);
		b = *(data+1);
		c = *(data+2);
		*(ptr+0) = a;
		*(ptr+1) = b;
		*(ptr+2) = c;
		*(ptr+3) = (a|b|c);
		ptr += 4;
		data += 3;
	}
	return newdata;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, WINDOW_WIDTH, 0, WINDOW_HEIGHT, -1, 1);

	//added for background
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_FOG);
	glDisable(GL_CULL_FACE);
	//clear the screen
	glClearColor(1.0, 1.0, 1.0, 1.0);
	backgroundImage = ppm6GetImage("./images/background.ppm");
	backgroundTransImage = ppm6GetImage("./images/backgroundTrans.ppm");
    gameoverbgImage = ppm6GetImage("./images/gameoverbg.ppm");
	


	//-------------------------------------------------------------------
	//bullet
	glGenTextures(1, &bulletTexture);
	bulletImage = ppm6GetImage("./images/bullet.ppm");
	int w3 = bulletImage->width;
	int h3 = bulletImage->height;
	glBindTexture(GL_TEXTURE_2D, bulletTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w3, h3, 0, GL_RGB, GL_UNSIGNED_BYTE, bulletImage->data);
	//-------------------------------------------------------------------
	
	//-------------------------------------------------------------------
	//White duck score sprite	
	glGenTextures(1, &duckscoreTexture);
	duckscoreImage = ppm6GetImage("./images/duck_score_1.ppm");
	int w4 = duckscoreImage->width;
	int h4 = duckscoreImage->height;
	glBindTexture(GL_TEXTURE_2D, duckscoreTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w4, h4, 0, GL_RGB, GL_UNSIGNED_BYTE, duckscoreImage->data);
	//-------------------------------------------------------------------
	
	//-------------------------------------------------------------------
	//Red duck score sprite	
	glGenTextures(1, &duckscoreTexture2);
	duckscoreImage2 = ppm6GetImage("./images/duck_score_2.ppm");
	int w5 = duckscoreImage2->width;
	int h5 = duckscoreImage2->height;
	glBindTexture(GL_TEXTURE_2D, duckscoreTexture2);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w5, h5, 0, GL_RGB, GL_UNSIGNED_BYTE, duckscoreImage2->data);
	//-------------------------------------------------------------------
	
	//-------------------------------------------------------------------
	//duck sprite
	glGenTextures(1, &duckTexture);
	glGenTextures(1, &duckSil);
	duckImage = ppm6GetImage("./images/duck.ppm");
	int w = duckImage->width;
	int h = duckImage->height;
	glBindTexture(GL_TEXTURE_2D, duckTexture);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, duckImage->data);
	//-------------------------------------------------------------------
	
	//-------------------------------------------------------------------
	//duck sprite 2
	glGenTextures(1, &duckTexture2);
	glGenTextures(1, &duckSil2);
	duckImage2 = ppm6GetImage("./images/duck2.ppm");
	int w2 = duckImage2->width;
	int h2 = duckImage2->height;
	glBindTexture(GL_TEXTURE_2D, duckTexture2);
	//
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, w2, h2, 0, GL_RGB, GL_UNSIGNED_BYTE, duckImage2->data);
	//-------------------------------------------------------------------

	//-------------------------------------------------------------------
	//duck silhouette 
	glBindTexture(GL_TEXTURE_2D, duckSil);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	////must build a new set of data...
	unsigned char *silhouetteData = buildAlphaData(duckImage);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
	GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData);
	delete [] silhouetteData;
	//-------------------------------------------------------------------
	
	//-------------------------------------------------------------------
	//duck silhouette 2 
	glBindTexture(GL_TEXTURE_2D, duckSil2);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	////must build a new set of data...
	unsigned char *silhouetteData2 = buildAlphaData(duckImage2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w2, h2, 0,
	GL_RGBA, GL_UNSIGNED_BYTE, silhouetteData2);
	delete [] silhouetteData2;
	//-------------------------------------------------------------------

	//-------------------------------------------------------------------
	//background textures
	//create opengl texture elements
	glGenTextures(1, &backgroundTexture);
	glBindTexture(GL_TEXTURE_2D, backgroundTexture);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D, 0, 3, backgroundImage->width, backgroundImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, backgroundImage->data); 
	//-------------------------------------------------------------------

	//-------------------------------------------------------------------
//No genTextures for trans image?
	//forest transparent part
	glBindTexture(GL_TEXTURE_2D, backgroundTransTexture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    //must build a new set of data...
    int w6 = backgroundTransImage->width;
    int h6 = backgroundTransImage->height;
    unsigned char *ftData = buildAlphaData(backgroundTransImage);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w6, h6, 0, GL_RGBA, GL_UNSIGNED_BYTE, ftData);
    delete [] ftData;
	//-------------------------------------------------------------------
    
	//-------------------------------------------------------------------
    //gameover
	glGenTextures(1, &gameoverbgTexture);
    glBindTexture(GL_TEXTURE_2D, gameoverbgTexture);
    //
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, gameoverbgImage->width, gameoverbgImage->height, 0, GL_RGB, GL_UNSIGNED_BYTE, gameoverbgImage->data);
    //-------------------------------------------------------------------

	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
}

void makeDuck(Game *game, float x, float y)
{
	if(game->n >= MAX_DUCKS)
		return;
	struct timespec dt;
	clock_gettime(CLOCK_REALTIME, &dt);
	//double ts = timeDiff(&game->duckTimer, &dt);
	timeCopy(&game->duckTimer, &dt);
	int directionNum = rand() % 101;
	bool direction;
	if(directionNum >= 50)
		direction = true;
	else
		direction = false;
	std::cout << "makeDuck() " << x << " " << y << std::endl;
	Duck *d = new Duck;
	timeCopy(&d->time, &dt);
	d->s.center.x = x;
	d->s.center.y = y;
	d->s.center.z = 0.0;
	if(direction)
		d->velocity.x = 4.0 * (game->rounds * .5);
	else
		d->velocity.x = -4.0 * (game->rounds * .5);
	d->velocity.y = 4.0 * (game->rounds * .5);
	d->velocity.z = 0.0;
	d->s.width = 50.0;
	d->s.height = 50.0;
	//d->shot = false;
	d->next = game->duck;
	if(game->duck != NULL)
	{
		game->duck->prev = d;
	}
	game->duck = d;
	game->n++;
}

void check_resize(XEvent *e)
{
	if(e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if(xce.width != WINDOW_WIDTH || xce.height != WINDOW_HEIGHT)
	{
		reshape_window(xce.width, xce.height);
	}
}

void check_mouse(XEvent *e, Game *game)
{
	int y = WINDOW_HEIGHT - e->xbutton.y;

	Duck *d = game->duck;
	Duck *saved;
	//Rect r;
	//r.center = 0;
	struct timespec dt;
	clock_gettime(CLOCK_REALTIME, &dt);
	double ts;

	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		if (e->xbutton.button==1) {
			//Left button was pressed
			while(d)
			{	
				if(game->bullets <= 1)
				{
					saved = d->next;
					deleteDuck(game, d);
					d = saved;
					if(game->n == 1)
					{
						saved = d->next;
						deleteDuck(game, d);
						d = saved;
					}
					std::cout << "no bullets" << std::endl;
					std::cout << "game over" << std::endl;
					return;
				}
				if(e->xbutton.x >= d->s.center.x - d->s.width &&
						e->xbutton.x <= d->s.center.x + d->s.width &&
						y <= d->s.center.y + d->s.height &&
						y >= d->s.center.y - d->s.height)
				{
					ts = timeDiff(&d->time, &dt);
					if(ts < 1.5)
						game->score += 200;
					else
						game->score += 100;
					saved = d->next;
					deleteDuck(game, d);
					d = saved;
					game->bullets--;	
					std::cout << "shoot true" << std::endl;
					std::cout << "bullets = " << game->bullets << std::endl;
					game->duckShot++;
					return;
				}
				if(game->n == 2)
				{
					d = d->next;
					if(e->xbutton.x >= d->s.center.x - d->s.width &&
							e->xbutton.x <= d->s.center.x + d->s.width &&
							y <= d->s.center.y + d->s.height &&
							y >= d->s.center.y - d->s.height)
					{
						ts = timeDiff(&d->time, &dt);
						if(ts < 1.5)
							game->score += 200;
						else
							game->score += 100;
						saved = d->next;
						deleteDuck(game, d);
						d = saved;
						game->bullets--;
						std::cout << "shoot true" << std::endl;
						std::cout << "bullets = " << game->bullets << std::endl;
						game->duckShot++;
						return;
					}
					/*else
					  {
					  game->bullets--;
					  std::cout << "shoot false" << std::endl;
					  std::cout << "bullets = " << game->bullets << std::endl;
					  return;
					  }*/
				}
				game->bullets--;	
				std::cout << "shoot false" << std::endl;
				std::cout << "bullets = " << game->bullets << std::endl;
				d = d->next;
			}
		}
	}
	if (e->xbutton.button==3) {
		//Right button was pressed
		return;
	}
}

int check_keys(XEvent *e, Game *game)
{
	Duck *d = game->duck;
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		if (key == XK_Escape) {
			return 1;
		}
		//You may check other keys here.
		if(key == XK_1)
		{
			while(d)
			{
				deleteDuck(game, d);
				d = d->next;
			}
			gameover = false;
			game->rounds = 1;
			game->duckCount = 0;
			game->duckShot = 0;
			std::cout << "ROUND " << game->rounds << std::endl;
			game->bullets = 3;
			//makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			std::cout << "makeduck" << std::endl;
			std::cout << "bullets = " << game->bullets << std::endl;
			if(!game->oneDuck)
				game->oneDuck = true;
			else
				game->oneDuck = false;
			game->twoDuck = false;
		}
		if(key == XK_2)
		{
			while(d)
			{
				deleteDuck(game, d);
				d = d->next;
			}
			gameover = false;
			game->rounds = 1;
			game->duckCount = 0;
			game->duckShot = 0;
			std::cout << "ROUND " << game->rounds << std::endl;
			game->bullets = 3;
			//makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			//makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			std::cout << "2 makeduck" << std::endl;
			std::cout << "bullets = " << game->bullets << std::endl;
			if(!game->twoDuck)
				game->twoDuck = true;
			else
				game->twoDuck = false;
			game->oneDuck = false;
		}
	}
	return 0;
}

void movement(Game *game)
{
	Duck *d = game->duck;
	struct timespec dt;
	clock_gettime(CLOCK_REALTIME, &dt);

	if (game->n <= 0)
		return;

	while(d)
	{
		double ts = timeDiff(&d->time, &dt);
		//std::cout << ts << std::endl;
		if(ts > 5)
		{
			Duck *saved = d->next;
			deleteDuck(game, d);
			d = saved;
			//std::cout << "deleteDuck" << std::endl;
			continue;
		}

		if(d->s.center.x - d->s.width <= 0.0)
		{
			d->s.center.x = d->s.width + 0.1;
			d->velocity.x *= -1.0;
			std::cout << " off screen - left" << std::endl;
		}
		if(d->s.center.x + d->s.width >= WINDOW_WIDTH)
		{
			d->s.center.x = WINDOW_WIDTH - d->s.width - 0.1;
			d->velocity.x *= -1.0;
			std::cout << " off screen - right" << std::endl;
		}
		if(d->s.center.y - d->s.height <= game->floor)
		{
			d->s.center.y = game->floor + d->s.height + 0.1;
			d->velocity.y *= -1.0;
			std::cout << " off screen - down" << std::endl;
		}
		if(d->s.center.y + d->s.height >= WINDOW_HEIGHT)
		{
			d->s.center.y = WINDOW_HEIGHT - d->s.height - 0.1;
			d->velocity.y *= -1.0;
			std::cout << " off screen - up" << std::endl;
		}

		d->s.center.x += d->velocity.x;
		d->s.center.y += d->velocity.y;

		d = d->next;	
	}
}

void render(Game *game)
{
	float w, h, x, y;

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);

	if(background) {
		glBindTexture(GL_TEXTURE_2D, backgroundTexture);
		glBegin(GL_QUADS);
		glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
		glTexCoord2f(0.0f, 0.0f); glVertex2i(0, WINDOW_HEIGHT);
		glTexCoord2f(1.0f, 0.0f); glVertex2i(WINDOW_WIDTH, WINDOW_HEIGHT);
		glTexCoord2f(1.0f, 1.0f); glVertex2i(WINDOW_WIDTH, 0);
		glEnd();
	}
	//Transparent Background
//-----------------------------------------------------------------------------------
//Gameover variable goes on forever
	if(gameover) {
        glBindTexture(GL_TEXTURE_2D, gameoverbgTexture);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
        glTexCoord2f(0.0f, 0.0f); glVertex2i(0, WINDOW_HEIGHT);
        glTexCoord2f(1.0f, 0.0f); glVertex2i(WINDOW_WIDTH, WINDOW_HEIGHT);
        glTexCoord2f(1.0f, 1.0f); glVertex2i(WINDOW_WIDTH, 0);
        glEnd();
    }

	glDisable(GL_TEXTURE_2D);
	glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Drawing Shapes
	glColor3ub(255, 255, 255);
	glBegin(GL_LINES);
	glVertex2f(0.0, game->floor);
	glVertex2f(WINDOW_WIDTH, game->floor);
	glEnd();

	Rect r;
	r.bot = WINDOW_HEIGHT - 550;
	r.left = WINDOW_WIDTH - 715;
	r.center = 0;

	//Drawing Boxes
	Shape *s;

	//-------------------------------------------------------------------
	//Displaying bullets
	glColor3ub(90, 140, 90);
	s = &game->box[0];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	w = s->width;
	h = s->height;
	int num = 0, dist = 0;
	if (game->bullets == 3) {
		num = 3, dist = 80;
	}
	if (game->bullets == 2) {
		num = 2, dist = 60;
	}
	if (game->bullets == 1) {
		num = 1, dist = 40;
	}
	if (game->bullets == 0) {
		num = 0, dist = 0;
	}
	if (game->bullets != 0) {
		for (int i=0;i<num;i++) {
			bullet_sprite.pos[0] = dist - (i * 20);
			bullet_sprite.pos[1] = 40;
			bullet_sprite.pos[2] = 0;
			float wid = 10.0f;
			glPushMatrix();
			glTranslatef(bullet_sprite.pos[0], bullet_sprite.pos[1], bullet_sprite.pos[2]);
			glBindTexture(GL_TEXTURE_2D, bulletTexture);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.0f);
			glColor4ub(255,255,255,255);
			glBegin(GL_QUADS);
			if (bullet_sprite.vel[0] > 0.0) {
				glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
			} else {
				glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
			}
			glEnd();
			glPopMatrix();
			glDisable(GL_ALPHA_TEST);
		}
	}
	r.bot = s->height;
	r.left = s->width;
	glVertex2i(-w, -h);
	glVertex2i(-w, h);
	glVertex2i(w, h);
	glVertex2i(w, -h);
	glEnd();
	//ggprint16(&r , 16, 0x00ffffff, "%i", game->bullets);
	glPopMatrix();

	//-------------------------------------------------------------------
	//Displaying duck score sprites
	glColor3ub(90, 140, 90);
	s = &game->box[1];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	w = s->width;
	h = s->height;
	//Duck score sprites
	if (game->duckShot <= 10) {
		for (int i=0;i<=9;i++) {
			duckscore_sprite.pos[0] = 70 + (i * 25);
			duckscore_sprite.pos[1] = 42;
			duckscore_sprite.pos[2] = 0;
			float wid = 10.0f;
			glPushMatrix();
			glTranslatef(duckscore_sprite.pos[0], duckscore_sprite.pos[1], duckscore_sprite.pos[2]);
			glBindTexture(GL_TEXTURE_2D, duckscoreTexture);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.0f);
			glColor4ub(255,255,255,255);
			glBegin(GL_QUADS);
			if (duckscore_sprite.vel[0] > 0.0) {
				glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
			} else {
				glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
			}
			glEnd();
			glPopMatrix();
			glDisable(GL_ALPHA_TEST);
		}
	}
	int loop = 0;
	if (game->duckShot == 1) loop = 1;
	if (game->duckShot == 2) loop = 2;
	if (game->duckShot == 3) loop = 3;
	if (game->duckShot == 4) loop = 4;
	if (game->duckShot == 5) loop = 5;
	if (game->duckShot == 6) loop = 6;
	if (game->duckShot == 7) loop = 7;
	if (game->duckShot == 8) loop = 8;
	if (game->duckShot == 9) loop = 9;
	if (game->duckShot == 10) loop = 10;
	if (game->duckShot <= 9) {
		for (int i=0;i<loop;i++) {
			duckscore_sprite2.pos[0] = 70 + (i * 25);
			duckscore_sprite2.pos[1] = 42;
			duckscore_sprite2.pos[2] = 0;
			float wid = 10.0f;
			glPushMatrix();
			glTranslatef(duckscore_sprite2.pos[0], duckscore_sprite2.pos[1], duckscore_sprite2.pos[2]);
			glBindTexture(GL_TEXTURE_2D, duckscoreTexture2);
			glEnable(GL_ALPHA_TEST);
			glAlphaFunc(GL_GREATER, 0.0f);
			glColor4ub(255,255,255,255);
			glBegin(GL_QUADS);
			if (duckscore_sprite2.vel[0] > 0.0) {
				glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
			} else {
				glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
				glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
				glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
				glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
			}
			glEnd();
			glPopMatrix();
			glDisable(GL_ALPHA_TEST);
		}
	}
	

	
	r.bot = s->height;
	r.left = s->width;
	glVertex2i(-w, -h);
	glVertex2i(-w, h);
	glVertex2i(w, h);
	glVertex2i(w, -h);
	glEnd();
	//ggprint16(&r , 16, 0x00ffffff, "%i", game->duckShot);
	glPopMatrix();

	glColor3ub(90, 140, 90);
	s = &game->box[2];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	w = s->width;
	h = s->height;
	r.bot = s->height;
	r.left = s->width;
	glVertex2i(-w, -h);
	glVertex2i(-w, h);
	glVertex2i(w, h);
	glVertex2i(w, -h);
	glEnd();
	ggprint16(&r , 16, 0x00ffffff, "%i", game->score);
	glPopMatrix();

	glColor3ub(90, 140, 90);
	s = &game->box[3];
	glPushMatrix();
	glTranslatef(s->center.x, s->center.y, s->center.z);
	w = s->width;
	h = s->height;
	r.bot = s->height;
	r.left = s->width;
	glVertex2i(-w, -h);
	glVertex2i(-w, h);
	glVertex2i(w, h);
	glVertex2i(w, -h);
	glEnd();
	ggprint16(&r , 16, 0x00ffffff, "%i", game->rounds);
	glPopMatrix();


	duck_sprite.pos[0] = s->center.x;
	//glPushMatrix();
	if(game->oneDuck || game->twoDuck)
	{
		Duck *d = game->duck;
		if(!d && game->duckCount >= 10 && game->duckShot >= 6)
		{
			game->rounds++;
			game->duckCount = 0;
			game->duckShot = 0;
		}
		if(!d && game->duckCount >= 10 && game->duckShot < 6)
		{
			while(d)
			{
				deleteDuck(game, d);
				d = d->next;
			}
			game->oneDuck = false;
			game->twoDuck = false;
			gameover = true;
			std::cout << "GAME OVER" << std::endl;
		}
		if(!d && game->oneDuck && game->duckCount < 10)
		{
			game->bullets = 3;
			makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			game->duckCount++;
		}
		if(!d && game->twoDuck && game->duckCount < 9)
		{
			game->bullets = 3;
			makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			makeDuck(game, rand() % (WINDOW_WIDTH - 50 - 1) + 50 + 1, game->floor + 50 + 1);
			game->duckCount += 2;
		}
		glColor3ub(255, 255, 255);
		while(d)
		{
			w = d->s.width;
			h = d->s.height;
			x = d->s.center.x;
			y = d->s.center.y;
			glBegin(GL_QUADS);
			glVertex2f(x-w, y+h);
			glVertex2f(x-w, y-h);
			glVertex2f(x+w, y-h);
			glVertex2f(x+w, y+h);
			glEnd();
//This changes which duck it points to
			
			show_duck= 1;
			float wid = 50.0f;
			duck_sprite.pos[0] = x;
			duck_sprite.pos[1] = y;
			duck_sprite.pos[2] = s->center.z;
//not changing to second 
//duck dimensions
			d = d->next;
			duck_sprite2.pos[0] = x;
			duck_sprite2.pos[1] = y;
			duck_sprite2.pos[2] = s->center.z;
			

			if(show_duck) {
				glPushMatrix();
				glTranslatef(duck_sprite.pos[0], duck_sprite.pos[1], duck_sprite.pos[2]);
				//implement direction change 
				//if (show_duck) {
				//	glBindTexture(GL_TEXTURE_2D, duckTexture);
				//} 
//implement silhouette for the duck
//remove if statement and make
//a while loop to take in both duck 
//pointer variables
				if (silhouette) 
				{
//glBind makes the duck texture
					glBindTexture(GL_TEXTURE_2D, duckTexture);
					glEnable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GREATER, 0.0f);
					glColor4ub(255,255,255,255);
				}
				
				glBegin(GL_QUADS);
				if (duck_sprite.vel[0] > 0.0) {
					glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
					glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
					glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
					glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
				} else {
					glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
					glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
					glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
					glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
				}
				//Duck2
				glTranslatef(duck_sprite2.pos[0], duck_sprite2.pos[1], duck_sprite2.pos[2]);
				if (silhouette) 
				{
//glBind makes the duck texture
					glBindTexture(GL_TEXTURE_2D, duckTexture2);
					glEnable(GL_ALPHA_TEST);
					glAlphaFunc(GL_GREATER, 0.0f);
					glColor4ub(255,255,255,255);
				}
				
				glBegin(GL_QUADS);
				if (duck_sprite2.vel[0] > 0.0) {
					glTexCoord2f(0.0f, 1.0f); glVertex2i(-wid,-wid);
					glTexCoord2f(0.0f, 0.0f); glVertex2i(-wid, wid);
					glTexCoord2f(1.0f, 0.0f); glVertex2i( wid, wid);
					glTexCoord2f(1.0f, 1.0f); glVertex2i( wid,-wid);
				} else {
					glTexCoord2f(1.0f, 1.0f); glVertex2i(-wid,-wid);
					glTexCoord2f(1.0f, 0.0f); glVertex2i(-wid, wid);
					glTexCoord2f(0.0f, 0.0f); glVertex2i( wid, wid);
					glTexCoord2f(0.0f, 1.0f); glVertex2i( wid,-wid);
				}
				glEnd();
				glPopMatrix();
				
				//Transparent part
				if (trees && silhouette) {
                    glBindTexture(GL_TEXTURE_2D, backgroundTransTexture);
                    glBegin(GL_QUADS);
                    glTexCoord2f(0.0f, 1.0f); glVertex2i(0, 0);
                    glTexCoord2f(0.0f, 0.0f); glVertex2i(0, WINDOW_HEIGHT);
                    glTexCoord2f(1.0f, 0.0f); glVertex2i(WINDOW_WIDTH, WINDOW_HEIGHT);
                    glTexCoord2f(1.0f, 1.0f); glVertex2i(WINDOW_WIDTH, 0);
                    glEnd();
                }

				glDisable(GL_ALPHA_TEST);
			}
//Fix these!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//Possible issue when changing duck direction
//Duck will both change direction since they are the same
//Pointer variable
			
/*			
			glDisable(GL_TEXTURE_2D);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glEnable(GL_BLEND); 
			glDisable(GL_BLEND);
*/		
		}
	}
}

void deleteDuck(Game *game, Duck *node)
{
	if(node->prev == NULL)
	{
		if(node->next == NULL)
		{
			game->duck = NULL;
		}
		else
		{
			node->next->prev = NULL;
			game->duck = node->next;
		}
	}
	else
	{
		if(node->next == NULL)
		{
			node->prev->next = NULL;
		}
		else
		{
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
	}
	delete node;
	node = NULL;
	game->n--;
}
