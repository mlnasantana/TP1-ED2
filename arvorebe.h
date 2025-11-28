#ifndef ARVOREBE_H
#define ARVOREBE_H

#include "item.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define M_ORDEM 2
#define MAX_CHAVES (2 * M_ORDEM)

typedef struct No* Apontador;

typedef struct No {
    bool eh_folha;
    int n; // numero de chaves/itens
    Item itens[MAX_CHAVES + 1]; 
    Apontador filhos[MAX_CHAVES + 2];
    Apontador pai;
} No;


// --- Funções Públicas ---
void Inicializa(Apontador *arvore);
void Insere(Item item, Apontador *arvore, AnaliseExperimental *analise);
// CORREÇÃO: Nome da função alterado para ser único
bool Pesquisa_B_Estrela(Item *item, Apontador arvore, AnaliseExperimental *analise);
void Libera(Apontador *arvore);
void Imprime(Apontador arvore);

// --- Função Orquestradora ---
void RodaArvoreBEstrela(FILE *arq, int chave_procurada, int quantReg, bool P_flag);

#endif