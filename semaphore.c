#include <stdlib.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <pthread.h>

typedef struct
{
    _Atomic uint32_t count; // Contador do semáforo
} semaphore;

void sem_init(semaphore *sem, uint32_t value)
{
    sem->count = value;
}

void sem_dec(semaphore *sem)
{
    uint32_t current_value = atomic_load(&sem->count);

    // Caso em que tenta decrementar diretamente
    while (current_value > 0) {
        if (atomic_compare_exchange_strong(&sem->count, &current_value, current_value - 1)) {
            return;
        }
        current_value = atomic_load(&sem->count);
    }

    // Caso em que é necessário esperar
    while (1) {
        current_value = atomic_load(&sem->count);

        if (current_value > 0) {
            if (atomic_compare_exchange_strong(&sem->count, &current_value, current_value - 1))
                return;
        }

        syscall(SYS_futex, &sem->count, FUTEX_WAIT, 0);
    }
}

void sem_inc(semaphore *sem)
{
    atomic_fetch_add(&sem->count, 1);
    syscall(SYS_futex, &sem->count, FUTEX_WAKE, 1);
}

#define TAMANHO 10
int dados[TAMANHO];
size_t inserir = 0;
size_t remover = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

semaphore s_items;    // Itens disponíveis
semaphore s_espacos;  // Espaços disponíveis

void *produtor(void *arg) {
    int v;
    for (v = 1;; v++) {
        sem_dec(&s_espacos);  // Espera por espaço
        
        pthread_mutex_lock(&mutex);
        printf("Produzindo %d\n", v);
        dados[inserir] = v;
        inserir = (inserir + 1) % TAMANHO;
        pthread_mutex_unlock(&mutex);
        
        sem_inc(&s_items);   // Sinaliza novo item disponível
        usleep(500000);
    }
    return NULL;
}

void *consumidor(void *arg) {
    int id = *(int *)arg;
    for (;;) {
        sem_dec(&s_items);  // Espera por item disponível
        
        pthread_mutex_lock(&mutex);
        printf("%d: Consumindo %d\n", id, dados[remover]);
        remover = (remover + 1) % TAMANHO;
        pthread_mutex_unlock(&mutex);
        
        sem_inc(&s_espacos);  // Sinaliza novo espaço disponível
    }
    return NULL;
}

int main() {
    pthread_t threads[5];
    int id1 = 1, id2 = 2, id3 = 3, id4  = 4;
    
    sem_init(&s_items, 0);
    sem_init(&s_espacos, TAMANHO);

    pthread_create(&threads[0], NULL, produtor, NULL);
    pthread_create(&threads[1], NULL, consumidor, &id1);
    pthread_create(&threads[2], NULL, consumidor, &id2);
    pthread_create(&threads[3], NULL, consumidor, &id3);
    pthread_create(&threads[4], NULL, consumidor, &id4);

    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);
    pthread_join(threads[2], NULL);
    pthread_join(threads[3], NULL);
    pthread_join(threads[4], NULL);

    return 0;
}