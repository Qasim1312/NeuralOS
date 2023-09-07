# NeuralOS
This repository contains code for an operating system designed to implement and train neural networks using parallel processing on multi-core systems.
Key features of NeuralOS include:

Representing each layer of a neural network as a separate process
Treating each neuron as an individual thread
Utilizing inter-process communication (IPC) via pipes to exchange data between layers
Distributing computation across CPU cores to enable training of large neural networks
Implementing synchronization mechanisms like locks and semaphores
Performing parallel backpropagation to update weights across layers
The OS prototype is implemented in C++ using Unix system calls and libraries like fork(), pipes, threads and mutual exclusion primitives. It aims to demonstrate how an operating system architecture can be optimized for neural network training by distributing workload across available CPU cores.

The code implements a multi-process, multi-threaded approach for parallelizing neural network inference and training. This allows NeuralOS to efficiently leverage the processing power of modern multi-core CPUs.
