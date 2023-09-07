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

double* buff;
double* in_buff;
sem_t s1;
pthread_mutex_t m1;
int neurons = 0;
int layers = 0;
bool renew = 0;

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
		buff[bindex] = res[i];
		bindex++;
	}
	//result successfully stored in buffer and ready to be sent to next process
	
	pthread_mutex_lock(&m1);
	running++;
	
	if(running == 2)
	{
		sem_post(&s1);
	}
	
	pthread_mutex_unlock(&m1);
	pthread_exit(0);
}

int main()
{
	cout << "How many layers do you want (2 or more): ";
	cin >> layers;
	layers -= 2;
	cout << "How many neurons do you want in the inner and output layers: ";
	cin >> neurons;
	buff = new double[2*neurons]{0.0};
	in_buff = new double[2]{0.0};
	//input prepared for processing by neurons
	ifstream obj("input.txt");
	string in = "";
	getline(obj, in, '\n');
	string in1 = "", in2 = "";
	int iter = 0;
	//input 1
	while(in[iter] != ',')
	{
		in1 = in1 + in[iter];
		iter++;
	}
	iter++;
	//input 2
	while(iter < in.length())
	{
		if((in[iter] >= '1' && in[iter] <= '9') || in[iter] == '.')
		{
			in2 = in2 + in[iter];
		}
		iter++;
	}
	string w1 = "", w2 = "";
	getline(obj, w1, '\n');
	getline(obj, w2, '\n');
	obj.close();
	
	//preparing pipe to next layer
	char pname = '1';
	char npipe[6] = "pipe0";
	npipe[4] = pname;
	mkfifo(npipe, 0666);
	
	for(int nrun = 0; nrun < 2; nrun++)
	{	
	
	
		pid_t pid = fork();
		if(pid < 0)
			cout << "\n\nFORK FAILED\n\n";
		else if(pid == 0)
		{
			string ns = to_string(neurons);
			string nos = to_string(2);
			int lskip = 3;
			string lskips = to_string(lskip);
			char pn[1] = {pname};
			string lays = to_string(layers);
			if(layers == 0)
			{
				execlp("./outputlayer.exe", ns.c_str(), lays.c_str(), pn, nos.c_str(), lskips.c_str(), NULL);
			}
			else if(layers > 0)
			{
				execlp("./innerlayer.exe", ns.c_str(), lays.c_str(), pn, nos.c_str(), lskips.c_str(), NULL);
			}
		}
		
		//semaphore prepared
		sem_init(&s1, 0, 1);
		sem_wait(&s1);
		pthread_mutex_init(&m1, NULL);
		
		//threads
		ndata n1, n2;
		n1.input = stod(in1);
		n1.weights = w1;
		n1.num = 0;
		n2.input = stod(in2);
		n2.weights = w2;
		n2.num = 1;
		if(renew)
		{
			n1.input = in_buff[0];
			n2.input = in_buff[1];
		}
		pthread_t p1, p2;
		pthread_create(&p1, NULL, neuron, (void*)&n1);
		pthread_create(&p2, NULL, neuron, (void*)&n2);
		
		//waiting for threads
		sem_wait(&s1);
		sem_post(&s1);
		
		//threads have stored in buffer
		int fd;
		fd = open(npipe, O_WRONLY);
		write(fd, buff, sizeof(double)*2*neurons);
		close(fd);
		
		//waiting using named pipes
		fd = open(npipe, O_RDONLY);
		read(fd, in_buff, sizeof(double)*2);
		close(fd);
		cout << "\n\nInput Layer: " << in_buff[0] << ", " << in_buff[1] << endl << endl;
		renew = 1;
		sem_destroy(&s1);
		pthread_mutex_destroy(&m1);
		running = 0;
		//sending new input to next layer
	}
	unlink(npipe);
	//two forward passes through neural network
	cout << "\n\nNeural Network has completed task.\n\n";
	pthread_exit(0);
}
