/*
 * Sistema de Gerenciamento de UPA (Unidade de Pronto Atendimento)
 * Protocolo de Manchester - Classificacao de Risco (Brasil)
 *
 * Estruturas utilizadas:
 *   - Lista Encadeada  : registro geral de todos os pacientes (busca)
 *   - Fila de Prioridade (lista encadeada ordenada): fila de espera
 *   - Pilha            : historico de pacientes ja atendidos
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

/* ============================================================
   CLASSIFICACAO DE RISCO - Protocolo de Manchester (Brasil)
   ============================================================ */
typedef enum {
    VERMELHO = 1,   /* Emergencia         - Atendimento imediato  */
    LARANJA  = 2,   /* Muito Urgente      - Ate 10 minutos        */
    AMARELO  = 3,   /* Urgente            - Ate 60 minutos        */
    VERDE    = 4,   /* Pouco Urgente      - Ate 120 minutos       */
    AZUL     = 5    /* Nao Urgente        - Ate 240 minutos       */
} ClassificacaoRisco;

/* ============================================================
   ESTRUTURA DO PACIENTE
   ============================================================ */
typedef struct {
    int id;
    char nome[100];
    int idade;
    ClassificacaoRisco classificacao;
    char horaChegada[10];   /* HH:MM:SS */
} Paciente;

/* ============================================================
   NO DA FILA DE PRIORIDADE
   ============================================================ */
typedef struct NoFila {
    Paciente      *paciente;
    struct NoFila *prox;
} NoFila;

/* ============================================================
   FILA DE PRIORIDADE (lista encadeada ordenada por risco)
   Menor numero = maior prioridade; desempate por chegada (FIFO)
   ============================================================ */
typedef struct {
    NoFila *frente;
    int     tamanho;
} FilaPrioridade;

/* ============================================================
   NO DA PILHA (historico de atendimentos)
   ============================================================ */
typedef struct NoPilha {
    Paciente       *paciente;
    struct NoPilha *prox;
} NoPilha;

/* ============================================================
   PILHA
   ============================================================ */
typedef struct {
    NoPilha *topo;
    int      tamanho;
} Pilha;

/* ============================================================
   NO DA LISTA ENCADEADA (registro geral para busca)
   ============================================================ */
typedef struct NoLista {
    Paciente       *paciente;
    struct NoLista *prox;
} NoLista;

/* ============================================================
   LISTA ENCADEADA
   ============================================================ */
typedef struct {
    NoLista *cabeca;
    int      tamanho;
} ListaPacientes;

/* ============================================================
   ESTADO GLOBAL
   ============================================================ */
static int         proximoId = 1;
static FilaPrioridade fila;
static Pilha          historico;
static ListaPacientes registro;

/* ============================================================
   UTILITARIOS
   ============================================================ */
static const char *nomeRisco(ClassificacaoRisco c) {
    switch (c) {
        case VERMELHO: return "VERMELHO - Emergencia";
        case LARANJA:  return "LARANJA  - Muito Urgente";
        case AMARELO:  return "AMARELO  - Urgente";
        case VERDE:    return "VERDE    - Pouco Urgente";
        case AZUL:     return "AZUL     - Nao Urgente";
        default:       return "DESCONHECIDO";
    }
}

/* Codigos ANSI de cor por classificacao */
static const char *corRisco(ClassificacaoRisco c) {
    switch (c) {
        case VERMELHO: return "\033[1;31m";
        case LARANJA:  return "\033[38;5;208m";
        case AMARELO:  return "\033[1;33m";
        case VERDE:    return "\033[1;32m";
        case AZUL:     return "\033[1;34m";
        default:       return "\033[0m";
    }
}

#define RESET "\033[0m"
#define BOLD  "\033[1m"
#define CYAN  "\033[1;36m"

static void horaAtual(char *buf, size_t n) {
    time_t t = time(NULL);
    strftime(buf, n, "%H:%M:%S", localtime(&t));
}

static void pausar(void) {
    printf("\n  Pressione ENTER para continuar...");
    fflush(stdout);
    while (getchar() != '\n');
}

static void limparTela(void) {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

static void imprimirLinha(char c, int n) {
    for (int i = 0; i < n; i++) putchar(c);
    putchar('\n');
}

static void cabecalho(void) {
    printf(CYAN);
    imprimirLinha('=', 66);
    printf("       SISTEMA DE GERENCIAMENTO DE UPA\n");
    printf("       Protocolo de Manchester - Brasil\n");
    imprimirLinha('=', 66);
    printf(RESET);
}

static void imprimirPaciente(const Paciente *p) {
    const char *cor = corRisco(p->classificacao);
    printf("  ID:%-4d | %-28s | %3d anos | %s | %s%s%s\n",
        p->id, p->nome, p->idade, p->horaChegada,
        cor, nomeRisco(p->classificacao), RESET);
}

/* ============================================================
   ALOCACAO DE PACIENTE
   O registro (lista) e o dono da memoria do Paciente.
   Fila e Pilha guardam apenas ponteiros (sem posse).
   ============================================================ */
static Paciente *criarPaciente(const char *nome, int idade,
                                ClassificacaoRisco c) {
    Paciente *p = malloc(sizeof(Paciente));
    if (!p) { perror("malloc"); exit(EXIT_FAILURE); }
    p->id = proximoId++;
    strncpy(p->nome, nome, sizeof(p->nome) - 1);
    p->nome[sizeof(p->nome) - 1] = '\0';
    p->idade = idade;
    p->classificacao = c;
    horaAtual(p->horaChegada, sizeof(p->horaChegada));
    return p;
}

/* ============================================================
   LISTA ENCADEADA - OPERACOES
   ============================================================ */
static void listaInicializar(ListaPacientes *l) {
    l->cabeca  = NULL;
    l->tamanho = 0;
}

/* Insercao no inicio: O(1) */
static void listaInserir(ListaPacientes *l, Paciente *p) {
    NoLista *no = malloc(sizeof(NoLista));
    if (!no) { perror("malloc"); exit(EXIT_FAILURE); }
    no->paciente = p;
    no->prox     = l->cabeca;
    l->cabeca    = no;
    l->tamanho++;
}

/* Busca por ID: O(n) */
static Paciente *listaBuscarId(const ListaPacientes *l, int id) {
    for (NoLista *n = l->cabeca; n; n = n->prox)
        if (n->paciente->id == id) return n->paciente;
    return NULL;
}

/* Busca por nome (substring, case-insensitive): O(n) */
static int listaBuscarNome(const ListaPacientes *l, const char *busca) {
    char buscaL[100];
    size_t len = strlen(busca);
    for (size_t i = 0; i < len && i < sizeof(buscaL) - 1; i++)
        buscaL[i] = (char)tolower((unsigned char)busca[i]);
    buscaL[len] = '\0';

    int encontrados = 0;
    for (NoLista *n = l->cabeca; n; n = n->prox) {
        char nomeL[100];
        size_t nlen = strlen(n->paciente->nome);
        for (size_t i = 0; i < nlen && i < sizeof(nomeL) - 1; i++)
            nomeL[i] = (char)tolower((unsigned char)n->paciente->nome[i]);
        nomeL[nlen] = '\0';
        if (strstr(nomeL, buscaL)) {
            imprimirPaciente(n->paciente);
            encontrados++;
        }
    }
    return encontrados;
}

/* Libera nos e Pacientes (lista e dona): O(n) */
static void listaLiberar(ListaPacientes *l) {
    NoLista *atual = l->cabeca;
    while (atual) {
        NoLista *tmp = atual;
        atual = atual->prox;
        free(tmp->paciente);
        free(tmp);
    }
    l->cabeca  = NULL;
    l->tamanho = 0;
}

/* ============================================================
   FILA DE PRIORIDADE - OPERACOES
   ============================================================ */
static void filaInicializar(FilaPrioridade *f) {
    f->frente  = NULL;
    f->tamanho = 0;
}

static int filaVazia(const FilaPrioridade *f) {
    return f->frente == NULL;
}

/*
 * Enfileirar: insere em posicao ordenada por classificacao.
 * Pacientes com mesma classificacao entram no FINAL do grupo (FIFO).
 * Complexidade: O(n) no pior caso.
 */
static void enfileirar(FilaPrioridade *f, Paciente *p) {
    NoFila *no = malloc(sizeof(NoFila));
    if (!no) { perror("malloc"); exit(EXIT_FAILURE); }
    no->paciente = p;
    no->prox     = NULL;

    if (!f->frente || p->classificacao < f->frente->paciente->classificacao) {
        no->prox  = f->frente;
        f->frente = no;
    } else {
        NoFila *cur = f->frente;
        /* Avanca enquanto o proximo tem prioridade >= ao novo (FIFO para iguais) */
        while (cur->prox && cur->prox->paciente->classificacao <= p->classificacao)
            cur = cur->prox;
        no->prox  = cur->prox;
        cur->prox = no;
    }
    f->tamanho++;
}

/*
 * Desenfileirar: remove o primeiro elemento (maior prioridade).
 * Complexidade: O(1).
 */
static Paciente *desenfileirar(FilaPrioridade *f) {
    if (filaVazia(f)) return NULL;
    NoFila   *removido = f->frente;
    Paciente *p        = removido->paciente;
    f->frente = removido->prox;
    free(removido);
    f->tamanho--;
    return p;
}

/* Exibe fila sem remover: O(n) */
static void filaExibir(const FilaPrioridade *f) {
    if (filaVazia(f)) {
        printf("  Fila de espera vazia.\n");
        return;
    }
    printf("  Total na fila: %d paciente(s)\n\n", f->tamanho);
    int pos = 1;
    for (NoFila *n = f->frente; n; n = n->prox) {
        printf("  [%2d] ", pos++);
        imprimirPaciente(n->paciente);
    }
}

/* Libera apenas os nos (nao os Pacientes): O(n) */
static void filaLiberar(FilaPrioridade *f) {
    NoFila *atual = f->frente;
    while (atual) {
        NoFila *tmp = atual;
        atual = atual->prox;
        free(tmp);
    }
    f->frente  = NULL;
    f->tamanho = 0;
}

/* ============================================================
   PILHA - OPERACOES (historico de atendimentos)
   ============================================================ */
static void pilhaInicializar(Pilha *pi) {
    pi->topo     = NULL;
    pi->tamanho  = 0;
}

static int pilhaVazia(const Pilha *pi) {
    return pi->topo == NULL;
}

/* Empilhar: O(1) */
static void empilhar(Pilha *pi, Paciente *p) {
    NoPilha *no = malloc(sizeof(NoPilha));
    if (!no) { perror("malloc"); exit(EXIT_FAILURE); }
    no->paciente = p;
    no->prox     = pi->topo;
    pi->topo     = no;
    pi->tamanho++;
}

/* Exibe pilha sem remover: O(n) */
static void pilhaExibir(const Pilha *pi) {
    if (pilhaVazia(pi)) {
        printf("  Historico vazio.\n");
        return;
    }
    printf("  Total de pacientes atendidos: %d\n\n", pi->tamanho);
    int ordem = 1;
    for (NoPilha *n = pi->topo; n; n = n->prox) {
        printf("  [%2d] ", ordem++);
        imprimirPaciente(n->paciente);
    }
}

/* Libera apenas os nos (nao os Pacientes): O(n) */
static void pilhaLiberar(Pilha *pi) {
    NoPilha *atual = pi->topo;
    while (atual) {
        NoPilha *tmp = atual;
        atual = atual->prox;
        free(tmp);
    }
    pi->topo    = NULL;
    pi->tamanho = 0;
}

/* ============================================================
   ACOES DO MENU
   ============================================================ */
static ClassificacaoRisco selecionarRisco(void) {
    printf("\n  Classificacao de Risco (Protocolo de Manchester):\n\n");
    printf("  \033[1;31m[1] VERMELHO\033[0m - Emergencia         (atendimento imediato)\n");
    printf("  \033[38;5;208m[2] LARANJA \033[0m - Muito Urgente      (ate 10 minutos)\n");
    printf("  \033[1;33m[3] AMARELO \033[0m - Urgente            (ate 60 minutos)\n");
    printf("  \033[1;32m[4] VERDE   \033[0m - Pouco Urgente      (ate 120 minutos)\n");
    printf("  \033[1;34m[5] AZUL    \033[0m - Nao Urgente        (ate 240 minutos)\n");
    printf("\n  Escolha: ");

    int op;
    if (scanf("%d", &op) != 1 || op < 1 || op > 5) {
        while (getchar() != '\n');
        printf("  Opcao invalida. Classificando como AZUL.\n");
        return AZUL;
    }
    while (getchar() != '\n');
    return (ClassificacaoRisco)op;
}

static void acaoCadastrar(void) {
    char nome[100];
    int  idade;

    printf("\n  -- CADASTRO DE NOVO PACIENTE --\n\n");

    printf("  Nome completo: ");
    fflush(stdout);
    if (!fgets(nome, sizeof(nome), stdin) || nome[0] == '\n') {
        printf("  Nome invalido.\n");
        pausar();
        return;
    }
    nome[strcspn(nome, "\n")] = '\0';

    printf("  Idade: ");
    if (scanf("%d", &idade) != 1 || idade <= 0 || idade > 150) {
        printf("  Idade invalida.\n");
        while (getchar() != '\n');
        pausar();
        return;
    }
    while (getchar() != '\n');

    ClassificacaoRisco risco = selecionarRisco();

    Paciente *p = criarPaciente(nome, idade, risco);
    enfileirar(&fila, p);
    listaInserir(&registro, p);

    printf("\n  Paciente cadastrado com sucesso!\n  ");
    imprimirPaciente(p);
    pausar();
}

static void acaoChamar(void) {
    printf("\n  -- CHAMAMENTO DE PACIENTE --\n\n");

    if (filaVazia(&fila)) {
        printf("  Nao ha pacientes na fila de espera.\n");
        pausar();
        return;
    }

    Paciente *p = desenfileirar(&fila);
    empilhar(&historico, p);

    printf("  >> PACIENTE CHAMADO <<\n  ");
    imprimirPaciente(p);
    printf("\n  Pacientes restantes na fila: %d\n", fila.tamanho);
    pausar();
}

static void acaoVerFila(void) {
    printf("\n  -- FILA DE ESPERA --\n\n");
    filaExibir(&fila);
    pausar();
}

static void acaoHistorico(void) {
    printf("\n  -- HISTORICO DE ATENDIMENTOS --\n\n");
    pilhaExibir(&historico);
    pausar();
}

static void acaoBuscar(void) {
    printf("\n  -- BUSCA DE PACIENTE --\n\n");
    printf("  [1] Buscar por ID\n");
    printf("  [2] Buscar por Nome\n");
    printf("  Escolha: ");

    int op;
    if (scanf("%d", &op) != 1) {
        while (getchar() != '\n') { /* drena buffer */ }
        pausar();
        return;
    }
    while (getchar() != '\n') { /* drena buffer */ }

    if (op == 1) {
        printf("  ID: ");
        int id;
        if (scanf("%d", &id) != 1) {
            while (getchar() != '\n') { /* drena buffer */ }
            pausar();
            return;
        }
        while (getchar() != '\n');
        Paciente *p = listaBuscarId(&registro, id);
        printf("\n");
        if (p) { printf("  Paciente encontrado:\n  "); imprimirPaciente(p); }
        else   printf("  Paciente com ID %d nao encontrado.\n", id);
    } else if (op == 2) {
        printf("  Nome (ou trecho): ");
        char nome[100];
        if (!fgets(nome, sizeof(nome), stdin)) { pausar(); return; }
        nome[strcspn(nome, "\n")] = '\0';
        printf("\n");
        int n = listaBuscarNome(&registro, nome);
        if (n == 0) printf("  Nenhum paciente encontrado.\n");
        else        printf("\n  %d resultado(s).\n", n);
    } else {
        printf("  Opcao invalida.\n");
    }
    pausar();
}

static void acaoEstatisticas(void) {
    printf("\n  -- ESTATISTICAS --\n\n");
    printf("  Total cadastrados:    %d\n", registro.tamanho);
    printf("  Na fila de espera:    %d\n", fila.tamanho);
    printf("  Ja atendidos:         %d\n\n", historico.tamanho);

    if (!filaVazia(&fila)) {
        printf("  Proximo a ser chamado:\n  ");
        imprimirPaciente(fila.frente->paciente);

        int cont[6] = {0};
        for (NoFila *n = fila.frente; n; n = n->prox)
            cont[n->paciente->classificacao]++;

        printf("\n  Distribuicao na fila:\n");
        if (cont[VERMELHO]) printf("    \033[1;31mVERMELHO\033[0m: %d\n", cont[VERMELHO]);
        if (cont[LARANJA])  printf("    \033[38;5;208mLARANJA \033[0m: %d\n", cont[LARANJA]);
        if (cont[AMARELO])  printf("    \033[1;33mAMARELO \033[0m: %d\n", cont[AMARELO]);
        if (cont[VERDE])    printf("    \033[1;32mVERDE   \033[0m: %d\n", cont[VERDE]);
        if (cont[AZUL])     printf("    \033[1;34mAZUL    \033[0m: %d\n", cont[AZUL]);
    }
    pausar();
}

/* ============================================================
   LIBERACAO DE MEMORIA
   Modelo de posse: a lista e dona dos Paciente*.
   Fila e Pilha liberam apenas seus nos.
   ============================================================ */
static void liberarTudo(void) {
    filaLiberar(&fila);
    pilhaLiberar(&historico);
    listaLiberar(&registro);  /* libera Paciente* aqui */
}

/* ============================================================
   PROGRAMA PRINCIPAL
   ============================================================ */
int main(void) {
    filaInicializar(&fila);
    pilhaInicializar(&historico);
    listaInicializar(&registro);

    int op;
    do {
        limparTela();
        cabecalho();
        printf("\n");
        printf("  [1] Cadastrar novo paciente\n");
        printf("  [2] Chamar proximo paciente\n");
        printf("  [3] Ver fila de espera\n");
        printf("  [4] Historico de atendimentos\n");
        printf("  [5] Buscar paciente\n");
        printf("  [6] Estatisticas\n");
        printf("  [0] Sair\n");
        printf("\n");
        printf("  Na fila: %d | Atendidos: %d\n", fila.tamanho, historico.tamanho);
        printf("\n  Escolha: ");

        if (scanf("%d", &op) != 1) {
            op = -1;
            while (getchar() != '\n');
        } else {
            while (getchar() != '\n');
        }

        limparTela();
        cabecalho();

        switch (op) {
            case 1: acaoCadastrar();    break;
            case 2: acaoChamar();       break;
            case 3: acaoVerFila();      break;
            case 4: acaoHistorico();    break;
            case 5: acaoBuscar();       break;
            case 6: acaoEstatisticas(); break;
            case 0:
                printf("\n  Sistema encerrado. Ate logo!\n\n");
                break;
            default:
                printf("\n  Opcao invalida. Tente novamente.\n");
                pausar();
        }
    } while (op != 0);

    liberarTudo();
    return 0;
}
