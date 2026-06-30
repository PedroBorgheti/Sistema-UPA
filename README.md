# Sistema de Gerenciamento de UPA

**Disciplina:** Estruturas de Dados  
**NOME:**, Pedro Borgheti  


------------------------------------------------------------------------------------------

## Descrição

Sistema em linguagem C para gerenciar o atendimento de pacientes em uma **UPA (Unidade de Pronto Atendimento)**, implementando o **Protocolo de Manchester**, escala oficial de classificação de risco utilizada no Brasil.

O sistema permite cadastrar pacientes, chamá-los por ordem de prioridade clínica e pesquisar no registro geral — tudo utilizando listas encadeadas, fila de prioridade e pilha implementadas manualmente com ponteiros.

---

## Classificação de Risco — Protocolo de Manchester

A triagem de Manchester é o protocolo adotado pelo Ministério da Saúde do Brasil para classificação de risco em UPAs e prontos-socorros. Cada paciente recebe uma **pulseira colorida** que indica a urgência do seu atendimento:

| Cor       | Nível            | Tempo máximo de espera | Exemplos                                      |
|-----------|------------------|------------------------|-----------------------------------------------|
| 🔴 Vermelho | Emergência       | Imediato (0 min)       | Parada cardíaca, hemorragia grave, PCR        |
| 🟠 Laranja  | Muito Urgente    | Até 10 minutos         | Dor torácica, AVC, trauma grave               |
| 🟡 Amarelo  | Urgente          | Até 60 minutos         | Fratura, febre alta, dor moderada             |
| 🟢 Verde    | Pouco Urgente    | Até 120 minutos        | Ferimentos leves, gripe sem complicações      |
| 🔵 Azul     | Não Urgente      | Até 240 minutos        | Consultas de rotina, problemas crônicos leves |

A prioridade de atendimento segue rigorosamente esta ordem: Vermelho > Laranja > Amarelo > Verde > Azul. Pacientes com a mesma classificação são atendidos por ordem de chegada (FIFO).

---

## Estruturas de Dados Utilizadas

### 1. Lista Encadeada Simples (`ListaPacientes`)

Guarda **todos** os pacientes já cadastrados no sistema (incluindo os que já foram atendidos). É utilizada para busca por ID ou por nome.

- **Inserção:** O(1) — sempre no início da lista.
- **Busca por ID:** O(n) — percorre a lista até encontrar.
- **Busca por nome:** O(n) — percorre toda a lista com comparação *case-insensitive* por substring.

A lista é a **dona** da memória dos structs `Paciente` — é ela que libera os dados ao encerrar o programa.

### 2. Fila de Prioridade (`FilaPrioridade`)

Implementada como **lista encadeada ordenada** pela classificação de risco. Controla a ordem de chamada dos pacientes.

- **Inserção (`enfileirar`):** O(n) — percorre a lista para inserir na posição correta, mantendo a ordem de prioridade e, dentro do mesmo nível, a ordem de chegada (FIFO).
- **Remoção (`desenfileirar`):** O(1) — sempre remove o primeiro elemento (maior prioridade).
- **Exibição:** O(n).

### 3. Pilha (`Pilha`)

Armazena o **histórico** de pacientes já atendidos. Cada vez que um paciente é chamado (removido da fila), ele é empilhado no histórico.

- **Empilhar:** O(1).
- **Exibição:** O(n) — o mais recentemente atendido aparece primeiro.

### Modelo de posse de memória

```
criarPaciente()
      │
      ├─► enfileirar(&fila, p)     ← referência (sem posse)
      └─► listaInserir(&registro, p) ← DONO da memória

Quando chamado:
      fila ──desenfileirar()──► pilha  ← referência (sem posse)

Na saída:
      filaLiberar()   → libera apenas os nós NoFila*
      pilhaLiberar()  → libera apenas os nós NoPilha*
      listaLiberar()  → libera nós NoLista* E os Paciente*
```

Evita *double free* porque somente a lista desaloca os `Paciente*`.

---

## Bibliotecas Utilizadas

| Biblioteca   | Uso                                                     |
|--------------|---------------------------------------------------------|
| `<stdio.h>`  | Entrada/saída (`printf`, `scanf`, `fgets`)              |
| `<stdlib.h>` | Alocação dinâmica (`malloc`, `free`) e `exit`           |
| `<string.h>` | Manipulação de strings (`strncpy`, `strcspn`, `strstr`) |
| `<ctype.h>`  | Conversão de caracteres (`tolower`) para busca          |
| `<time.h>`   | Hora atual do sistema (`time`, `localtime`, `strftime`) |

Nenhuma biblioteca externa foi utilizada — apenas a biblioteca padrão C (libc).

---

## Funcionalidades

| Opção | Funcionalidade                  | Descrição                                              |
|-------|---------------------------------|--------------------------------------------------------|
| 1     | Cadastrar novo paciente         | Registra nome, idade, classificação e hora de chegada  |
| 2     | Chamar próximo paciente         | Remove o paciente de maior prioridade da fila          |
| 3     | Ver fila de espera              | Exibe todos os pacientes aguardando, em ordem          |
| 4     | Histórico de atendimentos       | Exibe (via pilha) os pacientes já chamados             |
| 5     | Buscar paciente                 | Busca por ID (exato) ou por nome (substring)           |
| 6     | Estatísticas                    | Totais e distribuição por cor na fila                  |

---

## Como Compilar e Executar

### Pré-requisitos

- GCC (ou outro compilador C compatível com C11)
- Make (opcional)

### Com Make

```bash
make          # compila
./upa         # executa
make clean    # remove o executável
```

### Sem Make

```bash
gcc -Wall -Wextra -pedantic -std=c11 -o upa upa.c
./upa
```

---

## Análise de Complexidade Temporal

| Operação                  | Estrutura            | Complexidade |
|---------------------------|----------------------|--------------|
| Cadastrar paciente        | Lista + Fila         | O(n)         |
| Chamar próximo paciente   | Fila de prioridade   | O(1)         |
| Exibir fila               | Fila de prioridade   | O(n)         |
| Exibir histórico          | Pilha                | O(n)         |
| Buscar por ID             | Lista encadeada      | O(n)         |
| Buscar por nome           | Lista encadeada      | O(n)         |
| Liberar toda a memória    | Lista + Fila + Pilha | O(n)         |

> O gargalo da inserção é O(n) pela necessidade de ordenar a fila de prioridade. Com uso de heap binária, a inserção seria O(log n), porém a implementação com lista encadeada foi escolhida para demonstrar o uso explícito de ponteiros e nós encadeados, conforme os requisitos da disciplina.

---

## Exemplo de Uso

```
Fila atual (após 3 cadastros):
  [ 1] ID:2  | Ana Lima      | 34 anos | 10:05:12 | VERMELHO - Emergencia
  [ 2] ID:1  | João Silva    | 67 anos | 10:04:50 | LARANJA  - Muito Urgente
  [ 3] ID:3  | Maria Costa   | 22 anos | 10:05:30 | VERDE    - Pouco Urgente

>> Chamar próximo: Ana Lima (VERMELHO) é atendida primeiro.
```
