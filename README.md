# Consumer-Producer: Shared Memory
Proyecto 1 de Sistemas Operativos

## Compilation instructions:
To build the system in the default bin directory open a terminal and run the command:
`
make
`
## Execution instructions:

To use the system you must first run the initializer program. Running the programs in a different order may result in the corresponding execution error. 

Navigate to the `bin` folder, where the programs were built. Only one system can be running at a certan time, due to the naming of the semaphores not being dynamic. 

### Initializer

To run the initializer execute the command:

`
./initializer -b BUFFER_NAME -s SIZE
`

where BUFFER_NAME is the given name for the buffer and SIZE corresponds to the amount of messages the buffer will hold. 

### Producer

To run the producer execute the command: 

`
./producer -b BUFFER_NAME -s TIME
`

where BUFFER_NAME is the given name for the buffer and TIME corresponds to the average wait time of the producer which follows an exponential distribution. 

### Consumer

To run the consumer execute the command: 

`
./consumer -b BUFFER_NAME -s TIME -m MODE
`

where BUFFER_NAME is the given name for the buffer, TIME corresponds to the average wait time of the producer which follows a Poisson distribution and MODE corresponds to the execution mode of the consumer, which can be either M for manual or A for automatic. 

### Finalizer

To run the finalizer execute the command: 

`
./finalizer -b BUFFER_NAME 
`
where BUFFER_NAME is the given name for the buffer.


if the `--f` flag is given, the finalizer will try to force a relase of the shared memory and shared semaphores which may result in undefined behaviour on the producer and consumer programs. 