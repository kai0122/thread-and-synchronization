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
int FILTER_SCALE;
int *filter_G;

pthread_mutex_t mutex[NUM_THREAD];

const char *inputfile_name[5] = {
	"input1.bmp",
	"input2.bmp",
	"input3.bmp",
	"input4.bmp",
	"input5.bmp"
};
const char *outputBlur_name[5] = {
	"Blur1.bmp",
	"Blur2.bmp",
	"Blur3.bmp",
	"Blur4.bmp",
	"Blur5.bmp"
};

/*
const char *outputSobel_name[5] = {
	"Sobel1.bmp",
	"Sobel2.bmp",
	"Sobel3.bmp",
	"Sobel4.bmp",
	"Sobel5.bmp"
};*/

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

unsigned char *pic_in, *pic_grey, *pic_blur, *pic_final;

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

unsigned char GaussianFilter(const int &w, const int &h)
{
	int tmp = 0;
	int ws = (int)sqrt((float)FILTER_SIZE);
	
	for (register int j = 0; j<ws; j++)
	{	
		register int NUM1 = j*ws;
		register int NUM2 = (h+j-(ws >> 1))*imgWidth + (w-(ws >> 1)); 
		for (register int i = 0; i<ws; i++)
		{
			tmp += filter_G[NUM1++] * pic_grey[NUM2++];
		}
	}
	
	tmp /= FILTER_SCALE;
	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

unsigned char NewGaussianFilter(const int &w, const int &h)
{
	int tmp = 0;
	int a, b;
	int ws = (int)sqrt((float)FILTER_SIZE);
	
	for (register int j = 0; j<ws; j++){
		register int NUM1 = j*ws;
		register int NUM2 = (h+j-(ws >> 1))*imgWidth + (w-(ws >> 1));
		for (register int i = 0; i<ws; i++)
		{
			
			a = w + i - (ws >> 1);
			b = h + j - (ws >> 1);

			// detect for borders of the image
			if (a<0 || b<0 || a>=imgWidth || b>=imgHeight){
				NUM1++;
				NUM2++;
				continue;
			}

			tmp += filter_G[NUM1++] * pic_grey[NUM2++];
		}
	}
	
	tmp /= FILTER_SCALE;
	if (tmp < 0) tmp = 0;
	if (tmp > 255) tmp = 255;
	return (unsigned char)tmp;
}

void *First(void *thread){
	int index = *((int*)(&thread));
	int block =  ceil(imgHeight/NUM_THREAD) + 1;
	int HEIGHT_MIN = block * index;
	int HEIGHT_MAX = HEIGHT_MIN + block;
	if(HEIGHT_MAX > imgHeight) HEIGHT_MAX = imgHeight;
	for (register int j = HEIGHT_MIN; j<HEIGHT_MAX; j++) {
		int N = j*imgWidth;
		for (register int i = 0; i<imgWidth; i++){
			pic_grey[N++] = RGB2grey(i, j);
		}
	}
	pthread_mutex_unlock(&mutex[index]);
	pthread_exit(NULL);
}

void *Second(void *thread){
	int index = *((int*)(&thread));
	int ws = (int)sqrt((float)FILTER_SIZE);

	int block =  ceil(imgHeight/NUM_THREAD) + 1;

	int HEIGHT_MIN = block * index;
	if(HEIGHT_MIN == 0) HEIGHT_MIN = (ws >> 1);

	int HEIGHT_MAX = HEIGHT_MIN + block;
	if(HEIGHT_MAX > imgHeight - (ws >> 1)) HEIGHT_MAX = imgHeight - (ws >> 1);

	for (register int j = HEIGHT_MIN; j<HEIGHT_MAX; j++) {
		for (register int i = 0 + (ws >> 1); i<imgWidth - (ws >> 1); i++){
			int NUM  = j*imgWidth + i;
			int NUM2 = (NUM << 1) + NUM;
			pic_blur[NUM] = GaussianFilter(i, j);
			pic_final[NUM2 + MYRED] = pic_blur[NUM];
			pic_final[NUM2 + MYGREEN] = pic_blur[NUM];
			pic_final[NUM2 + MYBLUE] = pic_blur[NUM];
		}
	}
	pthread_mutex_unlock(&mutex[index]);
	pthread_exit(NULL);
}

void *Third(void *thread){
	int flag = 0;
	int ws = (int)sqrt((float)FILTER_SIZE);

	for(register int j = 0;j < imgHeight;j++){
		if(flag == 0){
			if(j>=(ws >> 1) && j<imgHeight-(ws >> 1)) flag = 1;
		}
		else{
			if(j>=imgHeight-(ws >> 1)) flag = 0;
		}

		switch(flag){
			case 0:{
				for(register int i = 0;i < imgWidth;i++){
					int NUM2 = j*imgWidth + i;
					int NUM1 = (NUM2 << 1) + NUM2;
					pic_blur[NUM2] = NewGaussianFilter(i, j);
					pic_final[NUM1 + MYRED] = pic_blur[NUM2];
					pic_final[NUM1 + MYGREEN] = pic_blur[NUM2];
					pic_final[NUM1 + MYBLUE] = pic_blur[NUM2];
				}
				break;
			}
			default:{
				for(register int i = 0;i < (ws/2);i++){
					int NUM2 = j*imgWidth + i;
					int NUM1 = (NUM2 << 1) + NUM2;
					pic_blur[NUM2] = NewGaussianFilter(i, j);
					pic_final[NUM1 + MYRED] = pic_blur[NUM2];
					pic_final[NUM1 + MYGREEN] = pic_blur[NUM2];
					pic_final[NUM1 + MYBLUE] = pic_blur[NUM2];
				}
				for(register int i = imgWidth - (ws/2);i < imgWidth;i++){
					int NUM2 = j*imgWidth + i;
					int NUM1 = (NUM2 << 1) + NUM2;
					pic_blur[NUM2] = NewGaussianFilter(i, j);
					pic_final[NUM1 + MYRED] = pic_blur[NUM2];
					pic_final[NUM1 + MYGREEN] = pic_blur[NUM2];
					pic_final[NUM1 + MYBLUE] = pic_blur[NUM2];
				}
				break;
			}
		}
	}
	pthread_exit(NULL);
}

int main(void)
{
	//	*****************************************
	//			Mutex Initialization
	//	*****************************************

	for(int i=0;i<NUM_THREAD;i++)
		mutex[i] = PTHREAD_MUTEX_INITIALIZER;

	//	*****************************************
	// 			Read Mask fFile
	//	*****************************************

	FILE* mask;
	mask = fopen("mask_Gaussian.txt", "r");
	fscanf(mask, "%d", &FILTER_SIZE);
	fscanf(mask, "%d", &FILTER_SCALE);

	filter_G = new int[FILTER_SIZE];
	for (int i = 0; i<FILTER_SIZE; i++)
		fscanf(mask, "%d", &filter_G[i]);
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
		pic_blur = (unsigned char*)malloc(imgWidth*imgHeight*sizeof(unsigned char));
		pic_final = (unsigned char*)malloc(3 * imgWidth*imgHeight*sizeof(unsigned char));

		
		pthread_t thread[NUM_THREAD];
		pthread_t thread6;
		
		//	*****************************************
		//		Convert RGB Image to Grey Image
		//	*****************************************

		pthread_mutex_lock(&mutex[0]);
		pthread_create(&thread[0], NULL, First, (void *)0);
		pthread_mutex_lock(&mutex[1]);
		pthread_create(&thread[1], NULL, First, (void *)1);
		pthread_mutex_lock(&mutex[2]);
		pthread_create(&thread[2], NULL, First, (void *)2);	
		pthread_mutex_lock(&mutex[3]);
		pthread_create(&thread[3], NULL, First, (void *)3);	
		pthread_mutex_lock(&mutex[4]);
		pthread_create(&thread[4], NULL, First, (void *)4);
		pthread_mutex_lock(&mutex[5]);
		pthread_create(&thread[5], NULL, First, (void *)5);
		pthread_mutex_lock(&mutex[6]);
		pthread_create(&thread[6], NULL, First, (void *)6);
		pthread_mutex_lock(&mutex[7]);
		pthread_create(&thread[7], NULL, First, (void *)7);
		pthread_mutex_lock(&mutex[8]);
		pthread_create(&thread[8], NULL, First, (void *)8);
		pthread_mutex_lock(&mutex[9]);
		pthread_create(&thread[9], NULL, First, (void *)9);
		pthread_mutex_lock(&mutex[10]);
		pthread_create(&thread[10], NULL, First, (void *)10);
		pthread_mutex_lock(&mutex[11]);
		pthread_create(&thread[11], NULL, First, (void *)11);
		

		//	*****************************************
		//			Wait Until RGB Done
		//	*****************************************

		pthread_mutex_lock(&mutex[0]);
		pthread_mutex_lock(&mutex[1]);
		pthread_mutex_lock(&mutex[2]);
		pthread_mutex_lock(&mutex[3]);
		pthread_mutex_lock(&mutex[4]);
		pthread_mutex_lock(&mutex[5]);
		pthread_mutex_lock(&mutex[8]);
		pthread_mutex_lock(&mutex[9]);
		pthread_mutex_lock(&mutex[10]);
		pthread_mutex_lock(&mutex[11]);

		
		//	*********************************************
		//		Apply The Gaussian Filter to The Image
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
		pthread_create(&thread6  , NULL, Third , (void *)0);

		//	*****************************************
		//			Wait Until Gaussian Done
		//	*****************************************
		
		for(int i=0;i<NUM_THREAD;i++)
			pthread_join(thread[i], NULL);
		pthread_join(thread6, NULL);

		//	*****************************************
		// 			Write Output BMP File
		//	*****************************************

		bmpReader->WriteBMP(outputBlur_name[k], imgWidth, imgHeight, pic_final);

		//	*****************************************
		//			Free Memory Space
		//	*****************************************

		free(pic_in);
		free(pic_grey);
		free(pic_blur);
		free(pic_final);
	}

	return 0;
}
