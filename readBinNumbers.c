//
// Created by Luiz Ferreira.
//

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Argumento faltando: %s <nome do arquivo>\n", argv[0]);
        exit(1);
    }

    const char* filename = argv[1];

    FILE * file = fopen(filename, "rb");
    if(!file) {
        fprintf(stderr, "Erro na abertura do arquivo\n");
        exit(4);
    }

    long long int number;
    while (fread(&number, sizeof(long long int), 1, file)) {
        printf("\n%lld", number);
    }
    return 0;
}