//
//  main.c
//  Final Project CSC412
//
//  Created by Jean-Yves Hervé on 2017-05-01.
//  Copyright © 2017 Jean-Yves Hervé. All rights reserved.
//

#include <iostream>
#include <string>
#include <experimental/filesystem>
#include <vector>
#include <climits>
#include <mutex>
//
#include <cstdio>
#include <cstdlib>
#include <time.h>
//
#include "gl_frontEnd.h"
#include "imageIO_TGA.h"

using namespace std;

//==================================================================================
//	The variables defined here are for you to modify and add to
//==================================================================================
#define SEARCHSIZE	12
#define NUMTHREADS	10
#define OUT_PATH	"./Output/"
//==================================================================================
//	Function prototypes
//==================================================================================
void myKeyboard(unsigned char c, int x, int y);
void initializeApplication(int argc, char** argv);
int GetGrayScale(int row, int col, ImageStruct* image);
int GetHistogram(int row, int col, int size, ImageStruct* image);
int GetFocusedPixel(int row, int col, int size, std::vector<ImageStruct*> images);
void* GetAllFocused(void*);
//void SetOutput(std::vector<int> focusedpixel, ImageStruct* canvas, std::vector<ImageStruct*> images);
//==================================================================================
//	Application-level global variables
//==================================================================================

//	Don't touch. These are defined in the front-end source code
extern int	gMainWindow;

//	Don't rename any of these variables/constants
//--------------------------------------------------
unsigned int numLiveFocusingThreads = 0;		//	the number of live focusing threads

//	An array of C-string where you can store things you want displayed in the spate pane
//	that you want the state pane to display (for debugging purposes?)
//	Dont change the dimensions as this may break the front end
//	I preallocate the max number of messages at the max message
//	length.  This goes against some of my own principles about
//	good programming practice, but I do that so that you can
//	change the number of messages and their content "on the fly,"
//	at any point during the execution of your program, whithout
//	having to worry about allocation and resizing.
const int MAX_NUM_MESSAGES = 8;
const int MAX_LENGTH_MESSAGE = 32;
char** message;
int numMessages;
time_t launchTime;

//	This is the image that you should be writing into.  In this
//	handout, I simply read one of the input images into it.
//	You should not rename this variable unless you plan to mess
//	with the display code.
ImageStruct* imageOut;
mutex Permissionlock;
//==================================================================================
//	These are the functions that tie the computation with the rendering.
//	Some parts are "don't touch."  Other parts need your intervention
//	to make sure that access to critical section is properly synchronized
//==================================================================================

//	I can't see any reason why you may need/want to change this
//	function
void displayImage(GLfloat scaleX, GLfloat scaleY)
{
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPixelZoom(scaleX, scaleY);

	//--------------------------------------------------------
	//	stuff to replace or remove.
	//	Here I assign a random color to a few random pixels
	//--------------------------------------------------------
	/*int j = 0;
		

//	I make sure that my alpha channel is 255
			j++;
		}
		int newCol = (int) random()%0x100000000;;
		int* dest = (int*) imageOut->raster;
		dest[j*imageOut->width + i] = newCol;
		i++;
	}
	*/
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glDrawPixels(imageOut->width, imageOut->height,
				  GL_RGBA,
				  GL_UNSIGNED_BYTE,
				  imageOut->raster);

}


void displayState(void)
{
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	//--------------------------------------------------------
	//	stuff to replace or remove.
	//--------------------------------------------------------
	//	Here I hard-code a few messages that I want to see displayed in my state
	//	pane.  The number of live focusing threads will always get displayed
	//	(as long as you update the value stored in the.  No need to pass a message about it.
	time_t currentTime = time(NULL);
	numMessages = 3;
	sprintf(message[0], "System time: %ld", currentTime);
	sprintf(message[1], "Time since launch: %ld", currentTime-launchTime);
	sprintf(message[2], "I like cheese too");
	
	
	//---------------------------------------------------------
	//	This is the call that makes OpenGL render information
	//	about the state of the simulation.
	//	You may have to synchronize this call if you run into
	//	problems, but really the OpenGL display is a hack for
	//	you to get a peek into what's happening.
	//---------------------------------------------------------
	drawState(numMessages, message);
}

//	This callback function is called when a keyboard event occurs
//	You can change things here if you want to have keyboard input
//
void handleKeyboardEvent(unsigned char c, int x, int y)
{
	int ok = 0;
	
	switch (c)
	{
		//	'esc' to quit
		case 27:
			exit(0);
			break;

			//	If you want to do some cleanup, here would be the time to do it.
		//	Feel free to add more keyboard input, but then please document that
		//	in the report.
		
		default:
			ok = 1;
			break;
	}
	if (!ok)
	{
		//	do something?
	}
}

//------------------------------------------------------------------------
//	You shouldn't have to change anything in the main function.
//------------------------------------------------------------------------
int main(int argc, char** argv)
{
	initializeApplication(argc, argv);

	//	Even though we extracted the relevant information from the argument
	//	list, I still need to pass argc and argv to the front-end init
	//	function because that function passes them to glutInit, the required call
	//	to the initialization of the glut library.
	initializeFrontEnd(argc, argv, imageOut);
	
	//==============================================
	//	This is OpenGL/glut magic.  Don't touch
	//==============================================
	//	Now we enter the main loop of the program and to a large extend
	//	"lose control" over its execution.  The callback functions that
	//	we set up earlier will be called when the corresponding event
	//	occurs
	glutMainLoop();
	
	//	Free allocated resource before leaving (not absolutely needed, but
	//	just nicer.  Also, if you crash there, you know something is wrong
	//	in your code.
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		free(message[k]);
	free(message);
	
	//	This will probably never be executed (the exit point will be in one of the
	//	call back functions).
	return 0;
}


//==================================================================================
//	This is a part that you have to edit and add to, for example to
//	load a complete stack of images and initialize the output image
//	(right now it is initialized simply by reading an image into it.
//==================================================================================

typedef struct ImageSearchBlock{
	std::vector<ImageStruct*>* Images;
	int x0;
	int x1;
	int y0;
	int y1;
} ImageSearchBlock;

void initializeApplication(int argc, char**argv)
{

	//	I preallocate the max number of messages at the max message
	//	length.  This goes against some of my own principles about
	//	good programming practice, but I do that so that you can
	//	change the number of messages and their content "on the fly,"
	//	at any point during the execution of your program, whithout
	//	having to worry about allocation and resizing.
	message = (char**) malloc(MAX_NUM_MESSAGES*sizeof(char*));
	for (int k=0; k<MAX_NUM_MESSAGES; k++)
		message[k] = (char*) malloc((MAX_LENGTH_MESSAGE+1)*sizeof(char));
	//Read all the images in from the parameters (argv)
	std::cout<<"Reading Images: /n";
	std::vector<ImageStruct*>* images = new std::vector<ImageStruct*>();
	for(int c = 1; c < argc-1; c++){
		images->push_back(readTGA(argv[c]));
		std::cout<<"Image "<<argv[c]<<" read.\n";
	}

	// Initalize the imageOut with nothing and the same parameters as the input images
	imageOut = new ImageStruct(images->at(0)->width, images->at(0)->height, images->at(0)->type, images->at(0)->bytesPerRow);

	int startx = 0; 
	int endx = imageOut->width; 
	int starty = 0;
	int endy = imageOut->height/NUMTHREADS;
	for(int x = imageOut->height/NUMTHREADS; x <= imageOut->height; x=x+imageOut->height/NUMTHREADS){
		//Create a pthread variable	
		pthread_t tid;
		//struct to hold the images, start pixel and end pixel
		ImageSearchBlock* Data = new ImageSearchBlock{images, startx, endx, starty, endy};
		starty = starty + imageOut->height/NUMTHREADS;
		endy = endy + imageOut->height/NUMTHREADS;
		//Make a thread to get a focused image out of a set of images
		pthread_create(&tid, NULL, GetAllFocused, Data);
	}
	writeTGA(argv[argc-1], imageOut);
	srand((unsigned int) time(NULL));
	
	launchTime = time(NULL);
}
//generate random focused points on the output image.
//at each pixel find the most focused search box in all the images.
//then set the output images to be not the pixel but the search box around that pixel
void* GetAllFocused(void* ptr){
	numLiveFocusingThreads++;
	ImageSearchBlock* data = (ImageSearchBlock*)ptr;
	std::vector<ImageStruct*> images = *data->Images;
	srand(time(0));
	int i = data->y0;
	int j = data->x0;
	int k = 0;
	int mapij[data->x1][data->y1] = {0};
	int *outcolor = (int*)imageOut->raster;
	bool stay = true;
	while(stay){
		i = data->y0 + rand() % (data->y1-data->y0);
		j = rand() % (data->x1);
		//make sure it has not but set yet. 1 = set, 0 =  unset
		while(mapij[j][i] == 1){
			i = data->y0 + rand()% (data->y1-data->y0);
			j = rand() % (data->x1);
		}
		//mark it as set.
		mapij[j][i]=1;
		k++;
		//if program has set the number of pixels in the thread then set stay to be false to exit loop.
		if(k==data->x1*(data->y1-data->y0))stay=false;
		int focused = GetFocusedPixel(j,i,SEARCHSIZE, images);
		int *tempcolor = (int*)images[focused]->raster;
		Permissionlock.lock();
		for(int x = j -SEARCHSIZE/2; x <= j + SEARCHSIZE/2; x++){
			for(int y = i - SEARCHSIZE/2; y <= i +SEARCHSIZE/2; y++){
				 if(x>0 && x <images[0]->width && y > 0 && y <images[0]->height){
					outcolor[y * data->x1 +x] = tempcolor[y * data->x1 + x];
				}
			}
		}
		Permissionlock.unlock();
		//std::cout<<"Percent Finished = "<<((float)k/(images[0]->width*images[0]->height))*100<< '\n';
	}
	numLiveFocusingThreads--;
	return NULL;
}
//find the images that has the most in focus searchbox around a specific pixel.
int GetFocusedPixel(int row, int col, int size, std::vector<ImageStruct*> images){
	std::vector<int> greyscale;
	for(int c = 0; c < images.size(); c++){
		int temp = GetHistogram(row,col,size,images[c]);
		greyscale.push_back(temp);
	}
	int max = greyscale[0];
	int bestIndex = 0;
	for(int c = 1; c < greyscale.size(); c++){
		if (max < greyscale[c]) {
			max = greyscale[c];
			bestIndex = c;
		}
	}
	return bestIndex;
}
//calculate the range of color in a search box around the specific pixel.
int GetHistogram(int row, int col, int size, ImageStruct* image){
	int min = INT_MAX;
	int max = INT_MIN;
	int histagram[256]= {0};
	
	for(int x = row - SEARCHSIZE/2; x <= row + SEARCHSIZE/2; x++){
		for(int y = col - SEARCHSIZE/2; y <= col + SEARCHSIZE/2; y++){
			if(x>0 && x<image->width && y>0 && y<image->height){
				histagram[GetGrayScale(y,x,image)]++;
			}
		} 
	}
	//find the largest and the smallest color int he histagram and calculate the range.
	for(int c = 0; c < 256; c++){
		if(c > max && histagram[c] != 0) max = c;
		if(c < min && histagram[c] != 0) min = c;
	}
	return max-min;
}
/*
 I found another algorith that seems to work better at computing the grayscale called Luminance.
 	"Luminance is the standard algorithm used by image processing software (e.g., GIMP). 
	It is implemented by MATLAB's “rgb2gray” function, and it is frequently used 
	in computer vision"(California university engineering and computer science 
	professor, Christopher Kanan, and Garrison W. Cottrell)
*/
int GetGrayScale(int row, int col, ImageStruct* image){
	int* dest = (int*)image->raster;
        int red   = ((dest[row * image->width + col]      ) & 0xFF);
        int green = ((dest[row * image->width + col] >>  8) & 0xFF);
        int blue  = ((dest[row * image->width + col] >> 16) & 0xFF);
        int alpha = ((dest[row * image->width + col] >> 24) & 0xFF);
       	int gray  = (0.3*red+0.11*blue+0.59*green);

	//printf("(At[%d, %d] red:%d, green:%d, blue:%d, transparency:%d, gray:%d)\n", row, col, red, green, blue, alpha, gray);
	return gray;
}


