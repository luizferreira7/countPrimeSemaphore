//
// Created by Luiz Ferreira.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

//#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

// Semáforos
sem_t empty, full;
pthread_mutex_t mutex;

typedef struct {
    int bufferSize;
    long long int *buffer;
    char *filename;
} ProducerArgs;

typedef struct {
    int id;
    long long int *buffer;
    int bufferSize;
    int primes;
} ConsumerArgs;

int ehPrimo(long long int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    printf("\n%lld eh primo\n", n);
    return 1;
}

void insertBuffer(long long int value, long long int *buffer, int bufferSize) {
    static int in=0;

    sem_wait(&empty); //aguarda slot vazio para inserir
    pthread_mutex_lock(&mutex); //exclusao mutua entre produtores (aqui geral para log)

    buffer[in] = value;
    in = (in + 1) % bufferSize;

    for(int i=0; i<bufferSize; i++)
        printf("%lld ", buffer[i]);
    puts("");

    pthread_mutex_unlock(&mutex);
    sem_post(&full);
}

long long int consumeBuffer(long long int *buffer, int bufferSize) {
    long long int item;
    static int out=0;

    sem_wait(&full); //aguarda slot cheio para retirar
    pthread_mutex_lock(&mutex); //exclusao mutua entre consumidores (aqui geral para log)

    item = buffer[out];

    printf("\n\n\n%lld\n\n\n", item);

    buffer[out] = 0;
    out = (out + 1) % bufferSize;

    for(int i=0; i<bufferSize; i++)
        printf("%lld ", buffer[i]);
    puts("");

    pthread_mutex_unlock(&mutex);
    sem_post(&empty); //sinaliza um slot vazio

    return item;
}

void * producerTask(void * args) {

    ProducerArgs *pArgs = (ProducerArgs *) args;

    FILE * file = fopen(pArgs -> filename, "rb");
    if(!file) {
        fprintf(stderr, "Erro na abertura do arquivo\n");
        exit(4);
    }

    long long int number;
    while (fread(&number, sizeof(long long int), 1, file)) {
        insertBuffer(number, pArgs->buffer, pArgs->bufferSize);
    }

    for (int i = 0; i < pArgs -> bufferSize; i++) {
        insertBuffer(-1, pArgs -> buffer, pArgs -> bufferSize);
    }

    fclose(file);
    free(args);
    pthread_exit(NULL);
}

void * consumerTask(void * args) {

    ConsumerArgs *cArgs = (ConsumerArgs *) args;

    while (1) {
        long long int number = consumeBuffer(cArgs -> buffer, cArgs -> bufferSize);

        if (number == -1) {
            break;
        }

        cArgs -> primes += ehPrimo(number);
    }

    pthread_exit(cArgs);
}

int main(int argc, char* argv[]) {

    if (argc < 4) {
        fprintf(stderr, "Argumento faltando: %s <threads> <buffer size> <nome do arquivo>\n", argv[0]);
        exit(1);
    }

    int threadsNumber = atoi(argv[1]);
    int bufferSize = atoi(argv[2]);
    long long int *buffer = malloc(sizeof(long long int) * bufferSize);
    if (buffer == NULL) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    char* filename = argv[3];

    // Inicializa semáforos e mutex
    sem_init(&empty, 0, bufferSize);
    sem_init(&full, 0, 0);
    pthread_mutex_init(&mutex, NULL);

    pthread_t tid[threadsNumber + 1];

    // Cria thread do producer
    ProducerArgs *pArgs = malloc(sizeof(ProducerArgs));
    if (pArgs == NULL) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    pArgs -> bufferSize = bufferSize;
    pArgs -> buffer = buffer;
    pArgs -> filename = filename;

    if (pthread_create(&tid[0], NULL, producerTask, pArgs)) {
        printf("Erro na criacao do thread produtor\n");
        exit(3);
    }

    // Cria threads consumidoras
    for (int i = 1; i < threadsNumber; i++) {

        ConsumerArgs *cArgs = malloc(sizeof(ConsumerArgs));
        if (cArgs == NULL) {
            fprintf(stderr, "Erro ao alocar memória");
            exit(2);
        }

        cArgs -> id = i;
        cArgs -> buffer = buffer;
        cArgs -> bufferSize = bufferSize;
        cArgs -> primes = 0;

        if (pthread_create(&tid[i], NULL, consumerTask, cArgs)) {
            printf("Erro na criacao do thread produtor\n");
            exit(3);
        }
    }

    int id = 0;
    int primes = 0;

    int total = 0;

    for (int i = 1; i< threadsNumber; i++) {
        ConsumerArgs *tempArgs;
        if (pthread_join(tid[i], (void*) &tempArgs)) {
            fprintf(stderr, "Erro pthread_join()");
            exit(6);
        }

        if (tempArgs -> primes > primes) {
            id = tempArgs -> id;
            primes = tempArgs -> primes;
        }

        total += tempArgs -> primes;

        free(tempArgs);
    }

    printf("\nForam encontrados: %d primos!", total);
    printf("\nA thread vencedora foi: %d", id);

    free(buffer);
    sem_destroy(&empty);
    sem_destroy(&full);
    pthread_mutex_destroy(&mutex);

    return 0;
}
