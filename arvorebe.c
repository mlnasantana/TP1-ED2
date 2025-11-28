// OBRIGATÓRIO PARA ARQUIVOS > 2GB
#define _FILE_OFFSET_BITS 64 

#include "arvorebe.h"
#include <string.h> // Memmove
#include <time.h>

// --- FUNÇÕES AUXILIARES ---

TipoApontadorBE CriaNoBE(bool folha) {
    TipoApontadorBE novo = (TipoApontadorBE)calloc(1, sizeof(TipoPaginaBE));
    if (!novo) { perror("Erro malloc"); exit(1); }
    // Como usamos calloc, n=0 e ponteiros=NULL já estão feitos
    return novo;
}

void InicializaArvoreBEst(TipoApontadorBE* Arvore) {
    *Arvore = NULL;
}

void LiberaArvoreBE(TipoApontadorBE Ap) {
    if (Ap != NULL) {
        // Como não temos flag 'folha' explicita na struct original, 
        // verificamos se o primeiro filho é NULL para saber se é folha
        if (Ap->p[0] != NULL) { 
            for (int i = 0; i <= Ap->n; i++) {
                LiberaArvoreBE(Ap->p[i]);
            }
        }
        free(Ap);
    }
}

// --- PESQUISA ---
TipoOffset PesquisaBE(int chave, TipoApontadorBE Ap, int *comp) {
    if (Ap == NULL) return -1;

    int i = 0;
    while (i < Ap->n && chave > Ap->chaves[i]) {
        (*comp)++;
        i++;
    }

    (*comp)++; // Comparação final (igualdade)
    if (i < Ap->n && chave == Ap->chaves[i]) {
        return Ap->offsets[i];
    }

    return PesquisaBE(chave, Ap->p[i], comp);
}

// --- MANIPULAÇÃO DE NÓS (REDISTRIBUIÇÃO E SPLIT) ---

// Move uma chave do Filho[i] para o Filho[i-1] (Irmão Esquerda) via Pai
void RedistribuiEsquerda(TipoApontadorBE pai, int i) {
    TipoApontadorBE filho = pai->p[i];
    TipoApontadorBE irmaoEsq = pai->p[i-1];

    // 1. Desce a chave do Pai para o final do Irmão Esquerdo
    irmaoEsq->chaves[irmaoEsq->n] = pai->chaves[i-1];
    irmaoEsq->offsets[irmaoEsq->n] = pai->offsets[i-1];
    irmaoEsq->p[irmaoEsq->n + 1] = filho->p[0]; // Filho[0] do nó vira último do irmão
    irmaoEsq->n++;

    // 2. Sobe a primeira chave do Filho para o Pai
    pai->chaves[i-1] = filho->chaves[0];
    pai->offsets[i-1] = filho->offsets[0];

    // 3. Remove a primeira chave do Filho (shift left)
    memmove(&filho->chaves[0], &filho->chaves[1], (filho->n - 1) * sizeof(int));
    memmove(&filho->offsets[0], &filho->offsets[1], (filho->n - 1) * sizeof(TipoOffset));
    memmove(&filho->p[0], &filho->p[1], filho->n * sizeof(TipoApontadorBE));
    filho->n--;
}

// Move uma chave do Filho[i] para o Filho[i+1] (Irmão Direita) via Pai
void RedistribuiDireita(TipoApontadorBE pai, int i) {
    TipoApontadorBE filho = pai->p[i];
    TipoApontadorBE irmaoDir = pai->p[i+1];

    // 1. Abre espaço no início do Irmão Direito
    memmove(&irmaoDir->chaves[1], &irmaoDir->chaves[0], irmaoDir->n * sizeof(int));
    memmove(&irmaoDir->offsets[1], &irmaoDir->offsets[0], irmaoDir->n * sizeof(TipoOffset));
    memmove(&irmaoDir->p[1], &irmaoDir->p[0], (irmaoDir->n + 1) * sizeof(TipoApontadorBE));

    // 2. Desce a chave do Pai para o início do Irmão Direito
    irmaoDir->chaves[0] = pai->chaves[i];
    irmaoDir->offsets[0] = pai->offsets[i];
    irmaoDir->p[0] = filho->p[filho->n]; // Último ponteiro do filho vai pro irmão
    irmaoDir->n++;

    // 3. Sobe a última chave do Filho para o Pai
    pai->chaves[i] = filho->chaves[filho->n - 1];
    pai->offsets[i] = filho->offsets[filho->n - 1];

    // 4. Remove a última chave do Filho
    filho->n--;
}

// Divide o Filho[i] do Pai em dois (Split 1-para-2)
void SplitFilho(TipoApontadorBE pai, int i) {
    TipoApontadorBE filho = pai->p[i];
    TipoApontadorBE novo = CriaNoBE(false); // Flag não importa muito aqui pois copiamos
    novo->n = M_ESTRELA - 1;

    // 1. Copia metade direita do filho para o novo nó
    memcpy(novo->chaves, &filho->chaves[M_ESTRELA + 1], (M_ESTRELA - 1) * sizeof(int));
    memcpy(novo->offsets, &filho->offsets[M_ESTRELA + 1], (M_ESTRELA - 1) * sizeof(TipoOffset));
    memcpy(novo->p, &filho->p[M_ESTRELA + 1], M_ESTRELA * sizeof(TipoApontadorBE));

    // 2. Reduz tamanho do filho original
    filho->n = M_ESTRELA; 

    // 3. Abre espaço no Pai para receber a mediana
    memmove(&pai->p[i + 2], &pai->p[i + 1], (pai->n - i) * sizeof(TipoApontadorBE));
    memmove(&pai->chaves[i + 1], &pai->chaves[i], (pai->n - i) * sizeof(int));
    memmove(&pai->offsets[i + 1], &pai->offsets[i], (pai->n - i) * sizeof(TipoOffset));

    // 4. Sobe a mediana e conecta o novo nó
    pai->p[i + 1] = novo;
    pai->chaves[i] = filho->chaves[M_ESTRELA];
    pai->offsets[i] = filho->offsets[M_ESTRELA];
    pai->n++;
}

// --- INSERÇÃO EM NÓ NÃO CHEIO (LÓGICA PROATIVA B*) ---
void InsereNaoCheio(TipoApontadorBE x, int chave, TipoOffset offset, int *comp) {
    int i = x->n - 1;

    // Se é folha (p[0] == NULL indica folha nesta struct)
    if (x->p[0] == NULL) {
        while (i >= 0 && chave < x->chaves[i]) {
            (*comp)++;
            x->chaves[i + 1] = x->chaves[i];
            x->offsets[i + 1] = x->offsets[i];
            i--;
        }
        if (i >= 0) (*comp)++; 

        x->chaves[i + 1] = chave;
        x->offsets[i + 1] = offset;
        x->n++;
    } 
    else {
        // É nó interno
        while (i >= 0 && chave < x->chaves[i]) {
            (*comp)++;
            i--;
        }
        if (i >= 0) (*comp)++;
        i++; // Índice do filho para descer

        // --- AQUI ESTÁ A LÓGICA B* ---
        // Antes de descer, verificamos se o filho está cheio
        if (x->p[i]->n == MM_ESTRELA) {
            
            // 1. Tenta Redistribuir com Irmão Esquerdo (se existir e tiver espaço)
            if (i > 0 && x->p[i-1]->n < MM_ESTRELA) {
                RedistribuiDireita(x, i-1); // Move do Esq(i-1) pro Atual(i)? Não, move do Atual pro Esq?
                // Lógica corrigida:
                // Se o filho[i] está cheio, tentamos jogar pro [i-1] (Esq) ou [i+1] (Dir).
                
                // RedistribuiEsquerda: Joga DO filho[i] PARA filho[i-1]
                RedistribuiEsquerda(x, i); 
                
                // Agora filho[i] tem espaço, prossegue
                InsereNaoCheio(x->p[i], chave, offset, comp);
            }
            // 2. Tenta Redistribuir com Irmão Direito (se existir e tiver espaço)
            else if (i < x->n && x->p[i+1]->n < MM_ESTRELA) {
                // RedistribuiDireita: Joga DO filho[i] PARA filho[i+1]
                RedistribuiDireita(x, i);
                
                // Agora filho[i] tem espaço, prossegue
                InsereNaoCheio(x->p[i], chave, offset, comp);
            }
            // 3. Se vizinhos cheios, faz Split
            else {
                SplitFilho(x, i);
                // Após split, a chave mediana subiu. Precisamos ver qual dos dois novos filhos (i ou i+1) recebe a chave
                if (chave > x->chaves[i]) {
                    i++;
                }
                InsereNaoCheio(x->p[i], chave, offset, comp);
            }
        } else {
            // Filho não está cheio, desce normal
            InsereNaoCheio(x->p[i], chave, offset, comp);
        }
    }
}

// --- FUNÇÃO PRINCIPAL DE INSERÇÃO ---
void InsereBEst(int chave, TipoOffset offset, TipoApontadorBE* Arvore, int* comp) {
    if (*Arvore == NULL) {
        *Arvore = CriaNoBE(true);
        (*Arvore)->chaves[0] = chave;
        (*Arvore)->offsets[0] = offset;
        (*Arvore)->n = 1;
        return;
    }

    TipoApontadorBE raiz = *Arvore;

    // Se a raiz está cheia, ela precisa crescer (Split)
    // Raiz é exceção, não tem irmãos para redistribuir
    if (raiz->n == MM_ESTRELA) {
        TipoApontadorBE novaRaiz = CriaNoBE(false);
        novaRaiz->p[0] = raiz;
        *Arvore = novaRaiz;
        SplitFilho(novaRaiz, 0);
        InsereNaoCheio(novaRaiz, chave, offset, comp);
    } else {
        InsereNaoCheio(raiz, chave, offset, comp);
    }
}

// --- DRIVER ---
void RodaArvoreBEstrela(const char* nomeArq, int chave_procurada, int quantReg, bool P_flag) {
    TipoApontadorBE arvore;
    Item registro_temp;
    int comparacoes_criacao = 0;
    
    InicializaArvoreBEst(&arvore);
    
    FILE* arq = fopen(nomeArq, "rb");
    if (!arq) { perror("Erro"); return; }

    printf("\n========================================================\n");
    printf("METODO: 4 - ARVORE B* (ESTRELA) [Implementacao Real]\n");
    printf("========================================================\n");

    // --- FASE 1: CRIACAO ---
    clock_t inicio_criacao = clock();
    while(true) {
        TipoOffset posicao = ftello(arq);
        if (fread(&registro_temp, sizeof(Item), 1, arq) != 1) break;
        InsereBEst(registro_temp.chave, posicao, &arvore, &comparacoes_criacao);
    }
    clock_t fim_criacao = clock();
    double tempo_criacao = (double)(fim_criacao - inicio_criacao) / CLOCKS_PER_SEC;

    printf("--- FASE 1: CRIACAO DO INDICE ---\n");
    printf("Tempo Execucao: %.6f s\n", tempo_criacao);
    printf("Transferencias: %d (Leitura Sequencial)\n", quantReg); 
    printf("Comparacoes:    %d\n", comparacoes_criacao);

    // --- FASE 2: PESQUISA ---
    printf("\n--- FASE 2: PESQUISA ---\n");
    
    int comp_pesquisa = 0;
    int transf_pesquisa = 0;
    clock_t inicio_pesquisa = clock();
    
    if (chave_procurada != -1) {
        TipoOffset offset_encontrado = PesquisaBE(chave_procurada, arvore, &comp_pesquisa);
        
        bool encontrado = false;
        if (offset_encontrado != -1) {
            fseeko(arq, offset_encontrado, SEEK_SET);
            fread(&registro_temp, sizeof(Item), 1, arq);
            transf_pesquisa = 1; 
            encontrado = true;
        }
        
        clock_t fim_pesquisa = clock();
        double tempo_pesquisa = (double)(fim_pesquisa - inicio_pesquisa) / CLOCKS_PER_SEC;

        if (encontrado) {
            printf("[STATUS]: ITEM ENCONTRADO\n");
            printf("> Chave:  %d\n", registro_temp.chave);
            printf("> Dado1:  %ld\n", registro_temp.dado1);
            if(P_flag) printf("> Dado2:  %.50s...\n", registro_temp.dado2);
        } else {
            printf("[STATUS]: ITEM NAO ENCONTRADO (Chave %d)\n", chave_procurada);
        }

        printf("\n--- METRICAS DE PESQUISA ---\n");
        printf("Tempo Execucao: %.6f s\n", tempo_pesquisa);
        printf("Transferencias: %d\n", transf_pesquisa);
        printf("Comparacoes:    %d\n", comp_pesquisa);
    } 
    else {
         printf("Modo experimental (media) nao ativado.\n");
    }
    printf("========================================================\n");

    fclose(arq);
    LiberaArvoreBE(arvore);
}