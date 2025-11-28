#ifndef ARVOREBE_H
#define ARVOREBE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h> // Para off_t
#include "item.h"

// M alto para garantir árvore baixa
#define M_ESTRELA 100
#define MM_ESTRELA (2 * M_ESTRELA)

// Definição de tipo para suporte a arquivos grandes
typedef off_t TipoOffset;

typedef struct TipoPaginaBE* TipoApontadorBE;

typedef struct TipoPaginaBE {
    short n; // Quantidade de chaves
    int chaves[MM_ESTRELA];          
    TipoOffset offsets[MM_ESTRELA];  // Guarda ONDE está no arquivo
    TipoApontadorBE p[MM_ESTRELA + 1]; // Ponteiros para filhos
} TipoPaginaBE;

// Funções Públicas (Interface)
void InicializaArvoreBEst(TipoApontadorBE* Arvore);
void InsereBEst(int chave, TipoOffset offset, TipoApontadorBE* Ap, int* comp);
TipoOffset PesquisaBE(int chave, TipoApontadorBE Ap, int* comp);
void LiberaArvoreBE(TipoApontadorBE Ap);
void RodaArvoreBEstrela(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag);

#endif