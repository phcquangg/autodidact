#include <pthread.h>
#include <stdio.h>

pthread_t tid[2];
int counter;
pthread_mutex_t lock;

void* trythis (void* arg)
{
	pthread_mutex_lock(&lock);
	counter += 1;
	printf("Job %d started\n", counter);
	for (unsigned long i = 0; i < 0xFFFFFFFF; i++)
	printf("Job %d finished\n", counter);

	pthread_mutex_unlock(&lock);
	return NULL;
}

int main() {
	pthread_mutex_init(&lock, NULL);	

	for (int i = 0; i < 2; ++i)
		pthread_create(&tid[i], NULL, trythis, NULL);
	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	pthread_mutex_destroy(&lock);
	return 0;
}
