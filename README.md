# countPrimeSemaphore
 Atividades do Lab 6 da disciplina de Computação Concorrente
 
Objetivo: Implemente um programa concorrente onde UMA thread PRODUTORA carrega de um arquivo binario uma sequencia de N (N bastante grande) de numeros inteiros e os deposita em um buffer de tamanho M (M pequeno) — um de cada vez — que sera  compartilhado com threads CONSUMIDORAS. AS threads CONSUMIDORAS retiram os numeros — um de cada vez — e avaliam sua primalidade.

Ao final do programa (depois que os N numeros foram processados), devera ser retornado: (i) a quantidade de numeros primos encontrados (para avaliar a corretude do programa); e (ii) a thread consumidora VENCEDORA (aquela que encontrou a maior quantidade de primos).

Roteiro:
1. Comece gerando os arquivos (binarios) de teste, com N valores inteiros positivos e a quantidade de primos.
2. Implemente o programa que deve receber como entrada: a quantidade de threads consumidoras; o tamanho M do buffer e o nome do arquivo de entrada.
3. Verifique se o resultado final esta correto (quantidade de primos encontrados).
4. Execute o programa varias vezes, alterando os parametros de entrada e ateste sua corretude.
5. Nao se esqueça de garantir a organizaçao e modularizaçao do codigo.

## Instruções do Programa

Rode os comandos a seguir para compilar as classes necessárias

``` bash
❯ gcc -o gera generateBinNumbers.c -lm
```

``` bash
gcc -o main lab6.c -lm
```

OBS: Caso queira visualizar os numeros compile
``` bash
gcc -o read readBinNumbers.c -lm
```

1. Para criar o arquivo com os numeros execute:

``` bash
./gera <N> <nome_arquivo>
```

Exemplo:
``` bash
./gera 10000 primos
```

2. Após gerar os numeros matrizes execute o seguinte comando:
``` bash
./main <threads> <buffer> <arquivo> 
```

Exemplo:
``` bash
./main 4 10 primos
```

3. Execute o seguinte comando para ler os numeros:

``` bash
./read <arquivo>
```

Exemplo:
``` bash
./read primos
```
