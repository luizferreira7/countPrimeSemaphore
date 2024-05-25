//
// Created by Luiz Ferreira.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

// O macOS não aceita semaforos não nomeados, por isso é necessário usar o método sem_open() para iniciar em vez de sem_init()
// Assim usei o ifdef para verificar se o ambiente é macOS ou não
#ifdef __APPLE__
#define MODE 0644
sem_t *empty, *full;
#elif
sem_t empty, full;
#endif

//#define PRINT

// Declara mutex para exclusão mutua
pthread_mutex_t mutex;

// Struct usada para passar os dados para a thread produtora
typedef struct {
    char *filename;
    int threadsCount;
    int bufferSize;
    int *buffer;
} ProducerArgs;

// Struct usada para passar os dados para a thread consumidora
typedef struct {
    int id;
    int qtdPrimes;
    int bufferSize;
    int *buffer;
} ConsumerArgs;

// Método que verifica se numero é primo
int ehPrimo(int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Método que insere valor no buffer usando semaforo para sincronização e mutex para exclusão mutua
void insertBuffer(int value, int *buffer, int bufferSize) {

    static int in = 0;

#ifdef __APPLE__
    sem_wait(empty);
#elif
    sem_wait(&empty);
#endif

    pthread_mutex_lock(&mutex);

    buffer[in] = value;
    in = (in + 1) % bufferSize;

#ifdef PRINT
    printf("\n");
    printf("Thread produtora\n");
    for(int i=0; i<bufferSize; i++)
        printf("%lld ", buffer[i]);
    printf("\n");
#endif

    pthread_mutex_unlock(&mutex);

#ifdef __APPLE__
    sem_post(full);
#elif
    sem_post(&full);
#endif

}

// Método que consome valor do buffer usando semaforo para sincronização e mutex para exclusão mutua
int consumeBuffer(int id, int *buffer, int bufferSize) {

    int item;
    static int out = 0;

#ifdef __APPLE__
    sem_wait(full);
#elif
    sem_wait(&full);
#endif

    pthread_mutex_lock(&mutex);

    item = buffer[out];

    buffer[out] = 0;
    out = (out + 1) % bufferSize;

#ifdef PRINT
    printf("\n");
    printf("Thread consumidora %d\n", id);
    for(int i=0; i<bufferSize; i++)
        printf("%lld ", buffer[i]);
    printf("\n");
#endif

    pthread_mutex_unlock(&mutex);

#ifdef __APPLE__
    sem_post(empty);
#elif
    sem_post(&empty);
#endif

    return item;
}

// Método executado pela thread produtora
void * producerTask(void * args) {

    ProducerArgs *pArgs = (ProducerArgs *) args;

    // Prepara o arquivo para leitura
    FILE * file = fopen(pArgs -> filename, "rb");
    if (!file) {
        fprintf(stderr, "Erro na abertura do arquivo\n");
        exit(4);
    }

    // Le o primeiro numero do arquivo
    int number;
    fread(&number, sizeof(int), 1, file);

    // Loop para o produtor popular o buffer
    while (1) {
        // Variavel temporaria para permitir a leitura do proximo numero do arquivo
        int temp = number;

        // Le o proximo numero do arquivo
        int read = (int32_t) fread(&number, sizeof(int), 1, file);

        // So insere um novo valor no buffer enquanto existir uma nova linha para evitar que o numero total de primos seja inserido (ultima linha)
        if (read != 0) {
            insertBuffer(temp, pArgs -> buffer, pArgs -> bufferSize);
        } else {
            break;
        }
    }

    // Após inserir todos os valores do arquivo a thread produtora insere -1 que é condição de parada para as threads consumidoras
    for (int i = 0; i < pArgs -> threadsCount; i++) {
        insertBuffer(-1, pArgs -> buffer, pArgs -> bufferSize);
    }

    fclose(file);
    free(args);

    // Retorna o numero de primos do fim do arquivo
    pthread_exit( (void *) (size_t) number);
}

// Método executado pela thread consumidora
void * consumerTask(void * args) {

    ConsumerArgs *cArgs = (ConsumerArgs *) args;

    // Cria loop para consumir buffer
    while (1) {
        // Consome um valor do buffer
        int number = consumeBuffer(cArgs -> id, cArgs -> buffer, cArgs -> bufferSize);

        // Condição de parada
        if (number == -1) {
            break;
        }

        // Verifica se o numero é primo
        cArgs -> qtdPrimes += ehPrimo(number);
    }

    pthread_exit(cArgs);
}

int main(int argc, char* argv[]) {

    // Verifica argumentos passados
    if (argc < 4) {
        fprintf(stderr, "Argumento faltando: %s <threads> <buffer size> <nome do arquivo>\n", argv[0]);
        exit(1);
    }

    int threadsCount = atoi(argv[1]);
    int bufferSize = atoi(argv[2]);

    int *buffer = malloc(sizeof(int) * bufferSize);
    if (!buffer) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    char *filename = argv[3];

    // Inicializa semáforos nomeados para macOS
#ifdef __APPLE__
    sem_unlink("sem_empty");
    empty = sem_open("sem_empty", O_CREAT | O_EXCL, MODE, bufferSize);

    sem_unlink("sem_full");
    full = sem_open("sem_full", O_CREAT | O_EXCL, MODE, 0);
#elif
    // Inicializa semáforos
    sem_init(&empty, 0, bufferSize);
    sem_init(&full, 0, 0);
#endif

    // Inicializa mutex
    pthread_mutex_init(&mutex, NULL);

    // Inicializa threads
    pthread_t tid[threadsCount + 1];

    ProducerArgs *pArgs = malloc(sizeof(ProducerArgs));
    if (pArgs == NULL) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    pArgs -> bufferSize = bufferSize;
    pArgs -> buffer = buffer;
    pArgs -> filename = filename;
    pArgs -> threadsCount = threadsCount;

    // Cria thread produtora
    if (pthread_create(&tid[0], NULL, producerTask, pArgs)) {
        printf("Erro na criacao da thread produtora\n");
        exit(3);
    }

    // Cria threads consumidoras
    for (int i = 1; i < threadsCount + 1; i++) {

        ConsumerArgs *cArgs = malloc(sizeof(ConsumerArgs));
        if (cArgs == NULL) {
            fprintf(stderr, "Erro ao alocar memória");
            exit(2);
        }

        cArgs -> id = i;
        cArgs -> buffer = buffer;
        cArgs -> bufferSize = bufferSize;
        cArgs -> qtdPrimes = 0;

        if (pthread_create(&tid[i], NULL, consumerTask, cArgs)) {
            printf("Erro na criacao da thread consumidora\n");
            exit(3);
        }
    }

    int winnerThreadId = 0;
    int winnerThreadPrimes = 0;
    int totalPrimes = 0;

    // Aguarda término de execução das threads consumidoras
    for (int i = 1; i < threadsCount + 1; i++) {
        ConsumerArgs *cArgs;
        if (pthread_join(tid[i], (void*) &cArgs)) {
            fprintf(stderr, "Erro pthread_join()");
            exit(6);
        }

        printf("Thread %d: %d primos\n", cArgs -> id, cArgs -> qtdPrimes);

        // Verifica se thread encontrou mais primos que a ultima maior
        if (cArgs -> qtdPrimes > winnerThreadPrimes) {
            winnerThreadId = cArgs -> id;
            winnerThreadPrimes = cArgs -> qtdPrimes;
        }

        totalPrimes += cArgs -> qtdPrimes;

        free(cArgs);
    }

    // Aguarda o término da thread produtora
    int originalQtdPrimes = 0;
    if (pthread_join(tid[0], (void*) &originalQtdPrimes)) {
        fprintf(stderr, "Erro pthread_join()");
        exit(6);
    }

    if (originalQtdPrimes != totalPrimes) {
        fprintf(stderr, "Erro numero de primos encontrados difere do arquivo");
        exit(7);
    }

    printf("\nThread vencedora: %d\n", winnerThreadId);
    printf("Foram encontrados: %d primos!\n", totalPrimes);
    printf("O arquivo continha: %d primos!\n", originalQtdPrimes);

    free(buffer);
    pthread_mutex_destroy(&mutex);

#ifdef __APPLE__
    sem_close(empty);
    sem_close(full);
#elif
    sem_destroy(&empty);
    sem_destroy(&full);
#endif

    return 0;
}
