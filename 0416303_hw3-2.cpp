#include "bmpReader.h"
#include "bmpReader.cpp"
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

#define MYRED	2
#define MYGREEN 1
#define MYBLUE	0
#define NUM_THREAD 12
//#pragma GCC optimize("O3")

int imgWidth, imgHeight;
int FILTER_SIZE;
int *filter_GX;
int *filter_GY;

sem_t semaphore[NUM_THREAD];

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};

/*
const char *outputBlur_name[5] = {
	"Blur1.bmp",
	"Blur2.bmp",
	"Blur3.bmp",
	"Blur4.bmp",
	"Blur5.bmp"
};
*/

const char *outputSobel_name[5] = {
	"Sobel1.bmp",
	"Sobel2.bmp",
	"Sobel3.bmp",
	"Sobel4.bmp",
	"Sobel5.bmp"
};

struct arg_struct {
    int arg1;
    int arg2;
    arg_struct(int i,int j){
    	arg1 = i;
    	arg2 = j;
    }
    arg_struct(){

    }
};

unsigned char *pic_in, *pic_grey, *pic_image, *pic_final;

unsigned char RGB2grey(const int &w, const int &h)
{
	int N = (h*imgWidth + w);
	int num = (N << 1) + N;
	int TEMP = pic_in[num + MYRED] + pic_in[num + MYGREEN] + pic_in[num + MYBLUE];
	int tmp = TEMP / 3;

	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}


int SobelFilter(int w, int h)
{
	int tmpX = 0;
	int tmpY = 0;
	register int a, b;
	register int TEMP;
	register int TEMP2;
	int ws = (int)sqrt((float)FILTER_SIZE);
	for (register int j = 0; j<ws; j++){
		TEMP  = (h+j-(ws >> 1))*imgWidth + (w-((ws >> 1)));
		
		for (register int i = 0; i<ws; i++)
		{
			a = w + i - (ws >> 1);
			b = h + j - (ws >> 1);

			// detect for borders of the image
			if (a<0 || b<0 || a>=imgWidth || b>=imgHeight) continue;

			tmpX += filter_GX[j*ws + i] * pic_grey[b*imgWidth + a];
			tmpY += filter_GY[j*ws + i] * pic_grey[b*imgWidth + a];
		}
	}
	
	if(tmpX < 0) tmpX = 0;
	if(tmpY < 0) tmpY = 0;
	int temp = sqrt( tmpX*tmpX + tmpY*tmpY );
	if (temp > 255) return 255;
	else return temp;
}


void *First(void *thread){
	int index = *((int*)(&thread));
	int block =  ceil(imgHeight/NUM_THREAD) + 1;
	int HEIGHT_MIN = block * index;
	int HEIGHT_MAX = HEIGHT_MIN + block;
	if(HEIGHT_MAX > imgHeight) HEIGHT_MAX = imgHeight;
	register int TEMP;
	for (register int j = HEIGHT_MIN; j<HEIGHT_MAX; j++) {
		TEMP = j*imgWidth;
		for (register int i = 0; i<imgWidth; i++){
			pic_grey[TEMP++] = RGB2grey(i, j);
		}
	}
	
	sem_post(&semaphore[index]);
	pthread_exit(NULL);
}

void *Second(void *thread){
	int index = *((int*)(&thread));

	int block =  ceil(imgHeight/NUM_THREAD) + 1;
	int HEIGHT_MIN = block * index;
	int HEIGHT_MAX = HEIGHT_MIN + block;
	if(HEIGHT_MAX > imgHeight) HEIGHT_MAX = imgHeight;
	for (register int j = HEIGHT_MIN; j<HEIGHT_MAX; j++) {
		register int NUM2 = j*imgWidth;
		register int NUM1 = (NUM2 << 1) + NUM2;
		for (register int i = 0; i<imgWidth; i++){
			int NUM  = j*imgWidth + i;
			int NUM2 = (NUM << 1) + NUM;
			pic_image[NUM] = SobelFilter(i, j);
			pic_final[NUM2 + MYRED] = pic_image[NUM];
			pic_final[NUM2 + MYGREEN] = pic_image[NUM];
			pic_final[NUM2 + MYBLUE] = pic_image[NUM];
		}
	}

	sem_post(&semaphore[index]);
	pthread_exit(NULL);
}

int main(void)
{

	//	*****************************************
	//			Semaphore Initialization
	//	*****************************************

	for(int i=0;i<NUM_THREAD;i++){
		sem_init(&semaphore[i], 0, 1);
	}

	//	*****************************************
	// 			Read Mask fFile
	//	*****************************************

	FILE* mask;
	mask = fopen("mask_Sobel.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);

	filter_GX = new int[FILTER_SIZE];
	filter_GY = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_GX[i]);
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_GY[i]);
	fclose(mask);


	BmpReader* bmpReader = new BmpReader();
	for (int k = 0; k<5; k++){

		//	*****************************************
		// 			Read Input BMP File
		//	*****************************************
		
		pic_in = bmpReader->ReadBMP(inputfile_name[k], &imgWidth, &imgHeight);
		
		//	*****************************************
		// 		Allocate Space for Output Image
		//	*****************************************

		pic_grey = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_image = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));
		

		pthread_t thread[NUM_THREAD];
		
		//	*****************************************
		//		Convert RGB Image to Grey Image
		//	*****************************************

		sem_wait(&semaphore[0]);
		pthread_create(&thread[0], NULL, First, (void *)0);
		sem_wait(&semaphore[1]);
		pthread_create(&thread[1], NULL, First, (void *)1);
		sem_wait(&semaphore[2]);
		pthread_create(&thread[2], NULL, First, (void *)2);
		sem_wait(&semaphore[3]);
		pthread_create(&thread[3], NULL, First, (void *)3);
		sem_wait(&semaphore[4]);
		pthread_create(&thread[4], NULL, First, (void *)4);
		sem_wait(&semaphore[5]);
		pthread_create(&thread[5], NULL, First, (void *)5);
		sem_wait(&semaphore[6]);
		pthread_create(&thread[6], NULL, First, (void *)6);
		sem_wait(&semaphore[7]);
		pthread_create(&thread[7], NULL, First, (void *)7);
		sem_wait(&semaphore[8]);
		pthread_create(&thread[8], NULL, First, (void *)8);
		sem_wait(&semaphore[9]);
		pthread_create(&thread[9], NULL, First, (void *)9);
		sem_wait(&semaphore[10]);
		pthread_create(&thread[10], NULL, First, (void *)10);
		sem_wait(&semaphore[11]);
		pthread_create(&thread[11], NULL, First, (void *)11);		
		
		//	*****************************************
		//			Wait Until RGB Done
		//	*****************************************
		
		sem_wait(&semaphore[0]);
		sem_wait(&semaphore[1]);
		sem_wait(&semaphore[2]);
		sem_wait(&semaphore[3]);
		sem_wait(&semaphore[4]);
		sem_wait(&semaphore[5]);
		sem_wait(&semaphore[6]);
		sem_wait(&semaphore[7]);
		sem_wait(&semaphore[8]);
		sem_wait(&semaphore[9]);
		sem_wait(&semaphore[10]);
		sem_wait(&semaphore[11]);
		
		//	*********************************************
		//		Apply The Sobel Filter to The Image
		//	*********************************************

		pthread_create(&thread[0], NULL, Second, (void *)0);	
		pthread_create(&thread[1], NULL, Second, (void *)1);	
		pthread_create(&thread[2], NULL, Second, (void *)2);	
		pthread_create(&thread[3], NULL, Second, (void *)3);	
		pthread_create(&thread[4], NULL, Second, (void *)4);
		pthread_create(&thread[5], NULL, Second, (void *)5);
		pthread_create(&thread[6], NULL, Second, (void *)6);	
		pthread_create(&thread[7], NULL, Second, (void *)7);	
		pthread_create(&thread[8], NULL, Second, (void *)8);	
		pthread_create(&thread[9], NULL, Second, (void *)9);	
		pthread_create(&thread[10], NULL, Second, (void *)10);
		pthread_create(&thread[11], NULL, Second, (void *)11);
		
		//	*****************************************
		//			Wait Until Sobel Done
		//	*****************************************

		for(int i=0;i<NUM_THREAD;i++)
			pthread_join(thread[i], NULL);

		//	*****************************************
		// 			Write Output BMP File
		//	*****************************************

		bmpReader->WriteBMP(outputSobel_name[k], imgWidth, imgHeight, pic_final);

		//	*****************************************
		//			Free Memory Space
		//	*****************************************

		free(pic_in);
		free(pic_grey);
		free(pic_image);
		free(pic_final);

	}

	return 0;
}
