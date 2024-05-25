//
// Created by Luiz Ferreira.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// Metodo que verifica se é um numero primo
int ehPrimo(int n) {
    if (n <= 1) return 0;
    if (n == 2) return 1;
    if (n % 2 == 0) return 0;
    for (int i = 3; i <= sqrt(n); i += 2) {
        if (n % i == 0) return 0;
    }
    return 1;
}

// Metodo que escreve vetor dos numeros em um arquivo
void writeFile(const char* filename, int* numbers, int size, int primeCount) {

    FILE* file = fopen(filename, "wb");
    if (file == NULL) {
        fprintf(stderr, "Erro ao abrir o arquivo");
        exit(3);
    }

    for (int i = 0; i < size; i++) {
        fwrite(&numbers[i], sizeof(int), 1, file);
    }

    // Adiciona numero de primos ao final do arquivo
    fwrite(&primeCount, sizeof(int), 1, file);

    fclose(file);
}

// Metodo que gera vetor de numeros aleatorios
int* generateNumbers(int N) {

    int * numbers = malloc(sizeof(int) * (N + 1));
    if (numbers == NULL) {
        fprintf(stderr, "Erro ao alocar memória");
        exit(2);
    }

    srand(time(NULL));

    for (int i = 0; i < N; i++) {
        int number = rand();
        numbers[i] = number;
    }

    return numbers;
}

int main(int argc, char* argv[]) {

    if (argc < 3) {
        fprintf(stderr, "Argumento faltando: %s <quantidade de numeros> <nome do arquivo>\n", argv[0]);
        exit(1);
    }

    int N = atoi(argv[1]);
    const char* filename = argv[2];

    int * numbers = generateNumbers(N);

    int primeCount = 0;

    // Verifica quantos numeros do vetor sao primos
    for (int i = 0; i < N; i++) {
        primeCount += ehPrimo(numbers[i]);
    }

    printf("\n%d primos\n", primeCount);

    writeFile(filename, numbers, N, primeCount);

    free(numbers);

    return 0;
}
