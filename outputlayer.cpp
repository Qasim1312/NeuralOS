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
	double weight = stod(n1.weights);
	outbuff[n1.num] = weight*n1.input;
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
	outbuff = new double[neurons]{0.0};
	char pname = *argv[2];
	no = atoi(argv[3]);
	int lskip = stoi(argv[4]);
	buff = new double[no*neurons];
	//preparing connection to inner layer
	char npipe[6] = "pipe1";
	npipe[4] = pname;
	//connecting to previous pipe
	int fd = open(npipe, O_RDONLY);
	read(fd, buff, sizeof(double)*no*neurons);
	close(fd);
	
	//CODE TO UNIFY INPUT
	double* buff2 = new double[neurons]{0};
	for(int i = 0; i < no; i++)
	{
		for(int j = 0; j < neurons; j++)
		{
			buff2[j] += buff[(neurons * i)+j];
		}
	}
	
	/*
	//USED TO DISPLAY NEURON DATA
	cout << "No: " << no << endl;
	/////////////////////////////////////////////////////////////////////////////////////
	for(int i = 0; i < neurons; i++)
	{
		cout << buff2[i] << ' ';
	}
	cout << endl << endl;
	*/
	
	//semaphore prepared
	sem_init(&s1, 0, 1);
	sem_wait(&s1);
	pthread_mutex_init(&m1, NULL);
	//MOVING FILE POINTER
	ifstream obj1("input.txt");
	string weights = "";
	for(int i = 0; i < lskip; i++)
	{
		getline(obj1, weights, '\n');
	}
	lskip++;
	
	//threads
	ndata* nd = new ndata[neurons];
	pthread_t* ps = new pthread_t[neurons];
	for(int i = 0; i < neurons; i++)
	{
		getline(obj1, weights, '\n');
		nd[i].weights = weights;
		nd[i].num = i;
		nd[i].input = buff2[i];
		pthread_create(&(ps[i]), NULL, neuron, (void*)&(nd[i]));
	}
	//waiting for threads
	sem_wait(&s1);
	sem_post(&s1);
	obj1.close();
	//impulse transmitted to current neuron
	double output = 0;
	for(int i = 0; i < neurons; i++)
		output += (outbuff[i]);
	double new_output[2] = {0,0};
	
	//APPLYING FORMULA
	new_output[0] = (output*output + output + 1.0)/2.0;
	new_output[1] = (output*output - output)/2.0;
	
	//Storing output in result.txt file
	ofstream obj;
	obj.open("result.txt", ios::app);
	cout << "\n\nOutput: " << output << endl;
	obj << output;
	obj << '\n';
	cout << "Applying formula 1: " << new_output[0] << endl;
	obj << to_string(new_output[0]);
	obj << ", ";
	obj << to_string(new_output[1]);
	cout << "Applying formula 2: " << new_output[1] << endl;
	obj << '\n';
	obj.close();
	//pipe to back-track and wait
	fd = open(npipe, O_WRONLY);
	write(fd, new_output, sizeof(double)*2);
	close(fd);
	pthread_exit(0);
}
