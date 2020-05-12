#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>


#define NUMBERS_SIZE 20000


/*
	This is the hand over hand locking implementation. This is also ran on 2 cores. The traversal of the list consists of the program locking the current node that it's at, doing comparisons or whatever it needs to do, then lock the next node while unlocking the current node that its at.  

*/

typedef struct __node_t
{
	int key;
	pthread_mutex_t lock;
	struct __node_t *next;
	

} node_t;

typedef struct __list_t
{
	struct __node_t *head;
	pthread_mutex_t lock;
	int length;

} list_t;


typedef struct __my_arg
{
	list_t* list;
} my_arg_t;

// Used to initialize the list with certain values. 

void List_Init(list_t *L)
{

// I tried using the best performance implementation but it gave me segmentation errors so I had to implement this version. Creates a list with an initialized node. 

    	node_t *new = malloc(sizeof(node_t));
    	if (new == NULL)
 	{
        	perror("malloc");
        	return;
    	}
    	pthread_mutex_init(&new->lock, NULL);
    	new->next = NULL;
    	new->key = 0;
    	L->head = new;
	L->length = 0;

}


void List_Insert(list_t *L, int key)
{

	// No need to create a new node since one already resides in the list. 
	if (L->length == 0)
	{
		pthread_mutex_lock(&L->head->lock);
		L->head->key = key;
		L->length += 1;
		pthread_mutex_unlock(&L->head->lock);
		return;

	}

	else
	{
		node_t *newNodeInput = malloc(sizeof(node_t));
		if (newNodeInput == NULL)
		{
			perror("malloc");
			return;
		}

// Initialize lock.

		pthread_mutex_init(&newNodeInput->lock, NULL);
		
		pthread_mutex_lock(&L->head->lock);
		node_t *curr = L->head;
		
		while (curr != NULL)
		{
			if (curr->next == NULL)
			{
				newNodeInput->key = key;
				L->length += 1;
				curr->next = newNodeInput;
				break;

			}

			// Create a pointer to an address that contains the current node's lock. Used to implement hand over hand as I can just move to the next node and lock that node, then unlock. 
			pthread_mutex_t *lockHold = &curr->lock;
		
			curr = curr->next;
			pthread_mutex_lock(&curr->lock);
			pthread_mutex_unlock(lockHold);		

		}

		pthread_mutex_unlock(&curr->lock);

	}

}


/*
	Lookup to the end of the list. The way this works is that it traverses the list like an array, once it gets to the last node, it checks it and breaks out of the loop. Locking is included. 
*/

int List_Lookup(list_t *L, int key)
{
	int rv = -1; 
	pthread_mutex_lock(&L->head->lock);
	node_t *curr = L->head;

	
	// Just loop until the end of the list. 
	
	while (curr != NULL)
	{
// No need to continue if the length of the list is 0. The reason is that there's a node in the list when the INit function is called, but that node is not counted until Insert is called. 

		if (L->length == 0)
		{
			break;
		}


		else if (curr->key == key)
		{
			rv = 0;
			printf("hit\n");  
			break;
		}

		pthread_mutex_t *lockHold = &curr->lock;
		
		// Check to see if the node is the last node is the tail node. If so, just break out of the loop. 		
		if (curr->next == NULL)
		{
			break;
		}

		curr = curr->next;
		pthread_mutex_lock(&curr->lock);
		pthread_mutex_unlock(lockHold);
		
	}

	pthread_mutex_unlock(&curr->lock);


	return rv;

}


void* insertRandomInteger(void *arg)
{
	my_arg_t* listToGet = (my_arg_t*) arg;
	int numHold;
	for (int i = 0; i < 10000; i++)
	{
		numHold = rand();
		List_Insert(listToGet->list, numHold);

	}
	
	return (void*) 0;
}


void* listLookUp(void* arg)
{
	my_arg_t* listToGet = (my_arg_t*) arg;
	int numHold;
	for (int i = 0; i < 10000; i++)
	{
		numHold = rand();
		List_Lookup(listToGet->list, numHold);

	}

	return (void*) 0;

}


int main(int argc, char *argv[]) 
{


// This is where the program must input 20000 integers using 2 different threads where each thread inputs 10000 random integers. 

	list_t *linkedListTwoMillion = malloc(sizeof(list_t));
	list_t *linkedListwithLookup = malloc(sizeof(list_t));

        List_Init(linkedListTwoMillion);
	List_Init(linkedListwithLookup);

	struct timeval start, end;


	gettimeofday(&start, NULL);

	pthread_t p1, p2;

	my_arg_t arg1, arg2;

	arg1.list = linkedListTwoMillion;
	arg2.list = linkedListwithLookup;


	int rc1, rc2;

	rc1 = pthread_create(&p1, NULL, insertRandomInteger, &arg1);
	assert(rc1 == 0);

	rc2 = pthread_create(&p2, NULL, insertRandomInteger, &arg1);
	assert(rc2 == 0);


	pthread_join(p1, NULL);
	pthread_join(p2, NULL);


	gettimeofday(&end, NULL);

	long total_time = end.tv_sec * 1000 * 1000 + end.tv_usec - start.tv_sec * 1000 * 1000 - start.tv_usec;
	
	printf("Length: %d\n", linkedListTwoMillion->length);
	printf("First Objective: %ld microseconds\n", total_time);



// This is where one thread must input 10000 random integers into a linked list, and another thread must perfrom 10000 random integer lookups. (Concurrently)


	rc1, rc2 = 0;


	gettimeofday(&start, NULL);

	rc1 = pthread_create(&p1, NULL, insertRandomInteger, &arg2);
	assert(rc1 == 0);

	rc2 = pthread_create(&p2, NULL, listLookUp, &arg2);
	assert(rc2 == 0);

	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	
	gettimeofday(&end, NULL);	

	
	total_time = end.tv_sec * 1000 * 1000 + end.tv_usec - start.tv_sec * 1000 * 1000 - start.tv_usec;
	
	
	printf("Second Objective: %ld microseconds\n", total_time);


// This is objective 3, where there is a list of 1 million random integers, and each thread must perform a lookup of 1 million random integers of the linked list. (Concurrently).
// Updated each objective to 10000 per thread. 

	rc1, rc2 = 0;

	gettimeofday(&start, NULL);


	rc1 = pthread_create(&p1, NULL, listLookUp, &arg2);
	assert(rc1 == 0);

	rc2 = pthread_create(&p2, NULL, listLookUp, &arg2);
	assert(rc2 == 0);

	pthread_join(p1, NULL);
	pthread_join(p2, NULL);
	
	gettimeofday(&end, NULL);	

	
	total_time = end.tv_sec * 1000 * 1000 + end.tv_usec - start.tv_sec * 1000 * 1000 - start.tv_usec;
	

	
	printf("Third Objective: %ld microseconds\n", total_time);


	free(linkedListwithLookup);

	free(linkedListTwoMillion);

	return 0;

}

