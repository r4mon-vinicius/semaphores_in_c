#include <stdlib.h>
#include <stdatomic.h>
#include <stdint.h>
#include <unistd.h>
#include <linux/futex.h> 

typedef struct
{
    node *prox;
} node;

typedef struct
{
    _Atomic uint32_t count;
    node *head;
    node *tail;
} semaphore;

void sem_init(semaphore *sem, uint32_t value)
{
    sem->count = value;
    sem->head = NULL;
    sem->tail = NULL;
}

void sem_dec(semaphore *sem)
{
    node esp; 
    
    while (1) {
        if (sem->count > 0) {
            sem->count--;
            return;
        }

        esp.prox = NULL;

        if (sem->tail != NULL) {
            sem->tail->prox = &esp;
        } else {
            sem->head = &esp;
        }
        sem->tail = &esp;
    }
}

void sem_inc(semaphore *sem)
{
    node *esp;

    esp = sem->head;
    if (esp != NULL) {
        sem->head = esp->prox;
        if (sem->head == NULL) {
            sem->tail = NULL;
        }
    }

    sem->count++;
}