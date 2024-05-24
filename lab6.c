//
// Created by Luiz Ferreira.
//

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <math.h>

// O macOS não aceita semaforos sem nome, por isso é necessário usar o método sem_open() para iniciar em vez de sem_init()
// Assim usei o ifdef para verificar se o ambiente é macOS ou não
#ifdef __APPLE__
#define MODE 0644
sem_t *empty, *full, *global;
#elif
sem_t empty, full, global;
#endif


// Struct usada para passar os dados para a thread produtora
typedef struct {
    char *filename;
    int bufferSize;
    long long int *buffer;
} ProducerArgs;

// Struct usada para passar os dados para a thread consumidora
typedef struct {
    int id;
    int primes;
    int bufferSize;
    long long int *buffer;
} ConsumerArgs;

// Método que verifica se numero é primo
int ehPrimo(long long int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Método que insere valor no buffer usando semaforo para sincronização e exclusão mutua
void insertBuffer(long long int value, long long int *buffer, int bufferSize) {

    static int in = 0;

#ifdef __APPLE__
    sem_wait(empty);
#elif
    sem_wait(&empty);
#endif

    buffer[in] = value;
    in = (in + 1) % bufferSize;

//    for(int i=0; i<bufferSize; i++)
//        printf("%lld ", buffer[i]);
//    puts("");

#ifdef __APPLE__
    sem_post(full);
#elif
    sem_post(&full);
#endif

}

// Método que consome valor do buffer usando semaforo para sincronização e exclusão mutua
long long int consumeBuffer(long long int *buffer, int bufferSize) {

    long long int item;
    static int out = 0;

#ifdef __APPLE__
    sem_wait(full);
    sem_wait(global);
#elif
    sem_wait(&full);
    sem_wait(&global);
#endif

    item = buffer[out];

    buffer[out] = 0;
    out = (out + 1) % bufferSize;

//    for(int i=0; i<bufferSize; i++)
//        printf("%lld ", buffer[i]);
//    puts("");

#ifdef __APPLE__
    sem_post(global);
    sem_post(empty);
#elif
    sem_post(&global);
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
    long long int number;
    fread(&number, sizeof(long long int), 1, file);

    // Loop para o produtor popular o buffer
    while (1) {
        long long int temp = number;

        // Le o proximo numero do arquivo
        int read = (int32_t) fread(&number, sizeof(long long int), 1, file);

        // So insere um novo valor no buffer enquanto existir uma nova linha para evitar que o numero total de primos seja inserido (ultima linha)
        if (read != 0) {
            insertBuffer(temp, pArgs->buffer, pArgs->bufferSize);
        } else {
            break;
        }
    }

    // Após inserir todos os valores do arquivo a thread produtora insere -1 que é condição de parada para as threads consumidoras
    for (int i = 0; i < pArgs -> bufferSize; i++) {
        insertBuffer(-1, pArgs -> buffer, pArgs -> bufferSize);
    }

    fclose(file);
    free(args);

    // Retorna o numero de primos do fim do arquivo
    pthread_exit( (void *) number);
}

// Método executado pela thread consumidora
void * consumerTask(void * args) {

    ConsumerArgs *cArgs = (ConsumerArgs *) args;

    // Cria loop para consumir buffer
    while (1) {
        // Consome um valor do buffer
        long long int number = consumeBuffer(cArgs -> buffer, cArgs -> bufferSize);

        // Condição de parada
        if (number == -1) {
            break;
        }

        // Verifica se o numero é primo
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

#ifdef __APPLE__
    // Inicializa semáforos e mutex
    sem_unlink("sem_empty");
    empty = sem_open("sem_empty", O_CREAT | O_EXCL, MODE, bufferSize);

    sem_unlink("sem_full");
    full = sem_open("sem_full", O_CREAT | O_EXCL, MODE, 0);

    sem_unlink("sem_global");
    global = sem_open("sem_global", O_CREAT | O_EXCL, MODE, 1);
#elif
    sem_init(&empty, 0, bufferSize);
    sem_init(&full, 0, 0);
    sem_init(&global, 0, 1);
#endif

    pthread_t tid[threadsNumber + 1];

    ProducerArgs *pArgs = malloc(sizeof(ProducerArgs));
    if (pArgs == NULL) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    pArgs -> bufferSize = bufferSize;
    pArgs -> buffer = buffer;
    pArgs -> filename = filename;

    // Cria thread produtora
    if (pthread_create(&tid[0], NULL, producerTask, pArgs)) {
        printf("Erro na criacao do thread produtor\n");
        exit(3);
    }

    // Cria threads consumidoras
    for (int i = 1; i < threadsNumber + 1; i++) {

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

    // Aguarda término de execução das threads consumidoras
    for (int i = 1; i < threadsNumber + 1; i++) {
        ConsumerArgs *tempArgs;
        if (pthread_join(tid[i], (void*) &tempArgs)) {
            fprintf(stderr, "Erro pthread_join()");
            exit(6);
        }

        printf("Thread: %d, encontrou: %d\n", tempArgs -> id, tempArgs -> primes);

        // Verifica se thread encontrou mais primos que a ultima maior
        if (tempArgs -> primes > primes) {
            id = tempArgs -> id;
            primes = tempArgs -> primes;
        }

        total += tempArgs -> primes;

        free(tempArgs);
    }

    printf("\nA thread vencedora foi: %d\n", id);
    printf("Foram encontrados: %d primos!\n", total);

    // Aguarda o término da thread produtora
    int original = 0;
    if (pthread_join(tid[0], (void*) &original)) {
        fprintf(stderr, "Erro pthread_join()");
        exit(6);
    }

    printf("O arquivo continha: %d primos!\n", original);

    free(buffer);

#ifdef __APPLE__
    sem_close(empty);
    sem_close(full);
    sem_close(global);
#elif
    sem_destroy(&empty);
    sem_destroy(&full);
    sem_destroy(&global);
#endif

    return 0;
}
