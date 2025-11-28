#ifndef ARVOREB_H
#define ARVOREB_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "item.h" // Inclui Item e AnaliseExperimental

#define M 2
#define MAX (2 * M)

// Renomear a estrutura No para B_No
typedef struct B_No {
    int qtdChaves;
    int chaves[MAX];
    struct B_No* filhos[MAX + 1];
    bool folha;
} B_No;

typedef B_No* ArvoreB; // ArvoreB continuará a ser um ponteiro para B_No

// Renomear funções
B_No* B_criaNo(bool folha);
void B_arvoreInsere(ArvoreB* arvore, Item item, AnaliseExperimental* analise);
bool B_arvoreBusca(B_No* raiz, int chave, Item* resultado, AnaliseExperimental* analise);
void RodaArvoreB(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag);
void B_liberaArvoreB(ArvoreB arvore);


#endif // ARVOREB_H