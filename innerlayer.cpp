#include <stdio.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <iostream>
#include <fstream>
#include <cstdlib>
#include<sys/wait.h>
using namespace std;

int no = 0;	//number of neurons in previous layer
int neurons = 0;
int layers = 0;
double* buff = 0;
double* outbuff = 0;
sem_t s1;
pthread_mutex_t m1;

int running = 0;	//used to help simulate wait()

struct ndata	//data to send to neuron including input and weights
{
	string weights;
	double input;
	int num;
};

void* neuron(void* n)
{
	//calculating output
	ndata n1 = *((ndata*)n);
	string temp = "";
	int ind = -1;
	double* res = new double[neurons]{0};
	int rindex = 0;
	do
	{
		ind++;
		if(n1.weights[ind] == ' ')
		{
			ind++;
			temp = "";
		}
		if((n1.weights[ind] >= '0' && n1.weights[ind] <= '9') || n1.weights[ind] == '.' || n1.weights[ind] == '-')
		{
			temp += n1.weights[ind];
		}
		else if(n1.weights[ind] == ',' || n1.weights[ind] == '\0')
		{
			double tempd = stod(temp);
			temp = "";
			res[rindex] = n1.input*tempd;
			rindex++;
		}
		if(rindex == neurons)		//if neurons are less than all weights on a single line
			break;
	}while(n1.weights[ind] != '\0' && n1.weights[ind] != '\n');
	if(rindex < neurons)	//if weights are not enough for all neurons
	{
		for(int i = 0; i < neurons; i++)
		{
			if(rindex < neurons)
			{
				res[rindex] = res[i];
				rindex++;
			}
			else
				break;
		}
	}
	int bindex = n1.num*neurons;
	int max = bindex + neurons;
	for(int i = 0; bindex < max; i++)//int i = 0; i < max; i++
	{
		outbuff[bindex] = res[i];
		bindex++;
	}
	//result successfully stored in buffer and ready to be sent to next process
	
	pthread_mutex_lock(&m1);
	running++;
	
	if(running == neurons)
	{
		sem_post(&s1);
	}
	
	pthread_mutex_unlock(&m1);
	pthread_exit(0);
}

int main(int argc, char* argv[])
{
	neurons = stoi(argv[0]);
	layers = stoi(argv[1]);
	char pname = *argv[2];
	no = atoi(argv[3]);
	int lskip = stoi(argv[4]);
	buff = new double[no*neurons];
	outbuff = new double[neurons*neurons];
	char npipe[6] = "pipe1";
	npipe[4] = pname;
	pname++;
	
	//connecting to previous pipe
	int fd = open(npipe, O_RDONLY);
	read(fd, buff, sizeof(double)*no*neurons);
	close(fd);
	//impulse transmitted to current neuron
	
	//file handling
	ifstream obj("input.txt");
	string in = "";
	
	//loop to skip lines that have already been read
	for(int i = 0; i < lskip; i++)
	{
		getline(obj, in, '\n');
	}
	
	//reading weights for next layer:
	string* weights = new string[neurons];
	for(int i = 0; i < neurons; i++)
	{
		getline(obj, weights[i], '\n');
	}
	lskip += neurons;
	obj.close();
	//weights for neurons stored
	
	//pipe to next process
	char npipe2[6] = "pipe1";
	npipe2[4] = pname;
	mkfifo(npipe2, 0666);
	layers--;
	pid_t pid = fork();
	if(pid < 0)
		cout << "\n\nFORK FAILED\n\n";
	else if(pid == 0)
	{
		string ns = to_string(neurons);
		string lskips = to_string(lskip);
		char pn[1] = {pname};
		string lays = to_string(layers);
		if(layers == 0)
		{
			execlp("./outputlayer.exe", ns.c_str(), lays.c_str(), pn, ns.c_str(), lskips.c_str(), NULL);
		}
		else if(layers > 0)
		{
			execlp("./innerlayer.exe", ns.c_str(), lays.c_str(), pn, ns.c_str(), lskips.c_str(), NULL);
		}
	}
	
	//semaphore prepared
	sem_init(&s1, 0, 1);
	sem_wait(&s1);
	pthread_mutex_init(&m1, NULL);	
	
	//CODE TO UNIFY INPUT
	double* buff2 = new double[neurons]{0};
	for(int i = 0; i < no; i++)
	{
		for(int j = 0; j < neurons; j++)
		{
			buff2[j] += buff[(neurons * i)+j];
		}
	}
	//buff2 now contains as input for current layer
	
	/*
	//TO DISPLAY NEURON DATA
	cout << "No: " << no << endl;
	/////////////////////////////////////////////////////////////////////////////////////
	for(int i = 0; i < neurons; i++)
	{
		cout << buff2[i] << ' ';
	}
	cout << endl << endl;
	*/
	
	//threads
	ndata* nd = new ndata[neurons];
	pthread_t* ps = new pthread_t[neurons];
	for(int i = 0; i < neurons; i++)
	{
		nd[i].weights = weights[i];
		nd[i].num = i;
		nd[i].input = buff2[i];
		pthread_create(&(ps[i]), NULL, neuron, (void*)&(nd[i]));
	}
	//waiting for threads
	sem_wait(&s1);
	sem_post(&s1);
	delete[] nd;
	nd = 0;
	delete[] ps;
	ps = 0;
	
	//threads have stored in buffer
	int fd2;
	fd2 = open(npipe2, O_WRONLY);
	write(fd2, outbuff, sizeof(double)*neurons*neurons);
	close(fd2);
	double* buff3 = new double[2]{0};
	//waiting using named pipes
	fd2 = open(npipe2, O_RDONLY);
	read(fd2, buff3, sizeof(double)*2);
	close(fd2);
	//sending new input to next layer
	
	cout << "\n\nLayer " << pname << " : " << buff3[0] << ", " << buff3[1] << endl << endl;
	
	//pipe to back-track and wait
	fd = open(npipe, O_WRONLY);
	write(fd, buff3, sizeof(double)*2);
	close(fd);
	/*
	buff brings input data
	buff2 contains input data after it is unified
	outbuff contains data to send to next process
	buff3 contains data to back-track with.
	*/
	unlink(npipe2);
	pthread_exit(0);
}
