#include <unistd.h>
#include <pthread.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>

#define atendPart1 4
#define atendPart2 8

#define medidorPaciencia 2  //constante para determinar o quanto é passivel alguem passar na sua frente na fila
#define esperaReordena 4

#define priorGravida 4
#define priorIdoso 3
#define priorDefic 2
#define priorComum 1

/*
Precedencias:
    4                     3                         2                  1
 Grávida           ->   Idoso             ->   Deficiente       ->   Comum
(Maria e Marcos)       (Vanda e Valter)       (Paula e Pedro)       (Sueli e Silas)

*/

//tipo pessoa
typedef struct
{
    char nome[20];
    int prioridade;
    int espera;
} Pessoa;

//cidadãos
typedef struct
{
    Pessoa gravidas[2], idoso[2], deficiente[2], comum[2];
} cidadaos;

//lista pra Fila com prioridade
typedef struct celula *pointer;

typedef struct celula
{
    Pessoa pessoa;
    pointer prox;
} celula;

typedef struct
{
    pointer first;
} lista;
//fim lista fila com prioridade

//argumentos para threads

typedef struct
{
    lista FIFO;
    cidadaos formigopolis;
    int atendidos, esperando;
}argsT;

//fim argumentos para threads

//inicializando variaveis pthread
pthread_mutex_t trava_fila = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t espera_chegada = PTHREAD_COND_INITIALIZER;
pthread_cond_t espera_proximo = PTHREAD_COND_INITIALIZER;
//

//lidar com a lista
int Empt (lista Lista) {
    if (Lista.first == NULL) {
        return 1;
    }
}

void erroAloc(void* p) {
    if (p == NULL) {
        perror("Erro de alocação de memória! ");
        exit(1);
    }
}

void insereFilaPrior(lista *Lista, Pessoa pessoa) {
    pointer aux = Lista->first;
    pointer alocAUX = NULL, auxPosi = NULL;
    int aumentaPrioridade = 0, inseriu = 0;

    //caso a lista esteja vazia aloca e insere na 1° posição
    if (Empt(*Lista) == 1) {
        Lista->first = malloc(sizeof(*Lista->first));
        erroAloc(Lista->first);
        Lista->first->pessoa = pessoa;
        Lista->first->prox = NULL;
    }
    else if (aux->prox != NULL) {
        while (aux->prox != NULL)
        {
            if (aux->pessoa.prioridade < pessoa.prioridade) {
                auxPosi = aux;
                inseriu = 1;
                while (auxPosi->prox != NULL)
                {
                    auxPosi->pessoa.espera++;   //aumenta a espera de todos na fila
                    if (aux->pessoa.espera > 1) {
                        aumentaPrioridade = aux->pessoa.espera / medidorPaciencia;
                        aux->pessoa.espera = aux->pessoa.espera % medidorPaciencia;
                        aux->pessoa.prioridade += aumentaPrioridade;
                    }
                }

                alocAUX = malloc(sizeof(*Lista->first));
                erroAloc(alocAUX);
                alocAUX->pessoa = pessoa;
                alocAUX->prox = aux;
                aux = alocAUX;
            }

            aux = aux->prox;                    //avança o ponteiro
        }
        if (inseriu == 0) {
            aux->prox = malloc(sizeof(*Lista->first));
            erroAloc(aux->prox);                          //verifica se funcionou
            aux->prox->pessoa = pessoa;
            aux->prox->prox = NULL;
        }
    }
    else {
        if (aux->pessoa.prioridade < pessoa.prioridade) {
            alocAUX = malloc(sizeof(*Lista->first));    //aloca ponteiro auxiliar
            erroAloc(alocAUX);                          //verifica se funcionou
            alocAUX->pessoa = pessoa;                   //coloca a pessoa nesse ponteiro
            alocAUX->prox = aux;                        //aponta a proxima posição alocada para a posição atual

            //aumenta espera
            aux->pessoa.espera++;
            if (aux->pessoa.espera > 1) {
                aumentaPrioridade = aux->pessoa.espera / medidorPaciencia;
                aux->pessoa.espera = aux->pessoa.espera % medidorPaciencia;
                aux->pessoa.prioridade += aumentaPrioridade;
            }
            aux = alocAUX;                              //aponta a posição atual para a posição alocada
        }
        else {
            aux->prox = malloc(sizeof(*Lista->first));
            erroAloc(aux->prox);                          //verifica se funcionou
            aux->prox->pessoa = pessoa;
            aux->prox->prox = NULL;
        }
    }
}

void reordenaFila(lista *FIFO) {
    pointer aux = FIFO->first;
    pointer segPosProx, segPosAt;

    if (Empt(*FIFO) != 1) {
        while (aux != NULL)
        {
            if (aux->prox != NULL) {
                if (aux->prox->pessoa.prioridade > aux->pessoa.prioridade) {
                    segPosProx = aux->prox;
                    segPosAt = aux;
                    segPosAt->prox = segPosProx->prox;
                    segPosProx->prox = segPosAt;
                    aux = segPosProx;
                }
            }
            
            aux = aux->prox;
        }
    }
}

Pessoa removeFIFO(lista *Lista) {
    pointer auxFIFO = NULL;
    Pessoa atendida;

    if (Empt(*Lista) == 1) {
        printf("FIFO vazia!\n");
        exit(1);
    }

    atendida = Lista->first->pessoa;
    auxFIFO = Lista->first;
    Lista->first = Lista->first->prox;
    free(auxFIFO);
    return atendida;
}

Pessoa removeFIFOPReordena(lista *Lista, int i) {
    pointer auxFIFO = Lista->first, aux2 = NULL;
    Pessoa reordenada;
    int aux = 0;

    if (Empt(*Lista) == 1) {
        printf("FIFO vazia!\n");
        exit(1);
    }
    else if (aux == i) {
        reordenada = auxFIFO->pessoa;
        Lista->first = Lista->first->prox;
        free(auxFIFO);
        return reordenada;
    }
    else {
        while (aux < i)//vai até o ponteiro da posição anterior a célula que se irá reordenar
        {
            reordenada = auxFIFO->prox->pessoa; //pega o valor da pessoa que se ira reordenar para retorno
            aux2 = auxFIFO->prox;               //aponta para a célula a se liberar
            auxFIFO->prox = auxFIFO->prox->prox;//Faz a célula apontar para o novo próximo
            free(aux2);                         //libera
            return reordenada;
        }
    }
}

//fim lidar com a lista


void criaPessoas(Pessoa *p, char *nome, int prioridade) {
    strcpy(p->nome, nome);
    p->prioridade = prioridade;
    p->espera = 0;
}

//geral

//printa resultados:
void printaAtendido(char *nome) {
    printf("%s está sendo atendido...\n", nome);
    fflush(stdout);
}

void printaFila(lista FIFO) {
    pointer aux = FIFO.first;

    if (aux == NULL) {
        printf("\n");
        return;
    }
    else {
        while(aux != NULL) {
            printf("%c ", aux->pessoa.nome[0]);

            aux = aux->prox;
        }
        printf("\n");
    }
}
//fim printa resultados

void travaEADDFIFO(argsT *args, Pessoa p) {
    pthread_mutex_lock(&trava_fila);                        //trava a fila
        pthread_cond_wait(&espera_proximo, &trava_fila);    //espera alguem chegar
            printf("%s chegou na fila ", p.nome);
            insereFilaPrior(&(args->FIFO), p);
            printaFila(args->FIFO);
        
            //USADO SOMENTE PARA DEPURAÇÃO | FIRULAS
            args->esperando++;
        pthread_cond_signal(&espera_proximo);               //Sinaliza que o próximo pode se inserir na fila
    pthread_mutex_unlock(&trava_fila);  
}


//reordenar fila
void *reordFIFO(argsT *args) {
    pointer aux = args->FIFO.first;
    Pessoa pReinsert;
    int auxI;

    while (1)
    {
        pthread_mutex_lock(&trava_fila);                        //trava a fila
            auxI = 0;
            aux = args->FIFO.first;
            while (aux != NULL)
            {
                if (aux->prox != NULL) {
                    if (aux->pessoa.prioridade < aux->prox->pessoa.prioridade) {    //se a prioridade do que ta na frente é menor do que o que está atras, retira ele da fila e reinsere
                        pReinsert = removeFIFOPReordena(&(args->FIFO), auxI);
                        printf("%s realocado(a) na fila\n", pReinsert.nome);
                        fflush(stdout);
                        insereFilaPrior(&(args->FIFO), pReinsert);
                    }
                }

                aux = aux->prox;
                auxI++;
            }

        pthread_mutex_unlock(&trava_fila);

        if (args->FIFO.first == NULL) {
            break;
        }
        sleep(esperaReordena);
    }
}
//fim reordernar fila
//thread caixa
void* caixaEle(argsT *args) {
    while (1)
    {
        pthread_mutex_lock(&trava_fila);                        //trava a fila
                if (args->FIFO.first != NULL) {
                    printaAtendido(args->FIFO.first->pessoa.nome);   //printa quem está sendo atendido
                    removeFIFO(&(args->FIFO));                       //atende quem chegou
                
                    //USADO SOMENTE PARA DEPURAÇÃO | FIRULAS
                    args->atendidos++;
                    args->esperando--;
                    
                    if (args->atendidos == atendPart1) {
                        pthread_mutex_unlock(&trava_fila);
                        break;
                    }
                }
                else {
                    pthread_cond_signal(&espera_proximo);               //sinaliza para o próximo ir para o inicio da fila
                    //pthread_cond_wait(&espera_chegada, &trava_fila);   //espera alguem chegar
                }
        pthread_mutex_unlock(&trava_fila);                              //libera fila/
    }
    return NULL;
}
//fim thread caixa

//parte 1:
//thread clientes
void* pGravida(argsT *args) {
    travaEADDFIFO(args, args->formigopolis.gravidas[0]);
}

void *pIdosa(argsT *args) {
    travaEADDFIFO(args, args->formigopolis.idoso[0]);
}

void *pDefic(argsT *args) {
    travaEADDFIFO(args, args->formigopolis.deficiente[0]);  
}

void *pComum(argsT *args) {
   travaEADDFIFO(args, args->formigopolis.comum[0]);
}
//fim thread clientes

//principal
void parte1Trab() {
    pthread_t gravida, idoso, defic, comum, caixa, reordenador;
    argsT args;
    cidadaos formigopolis;
    //argsT args;
    args.atendidos = 0; args.esperando = 0;

    //criando as pessoas
        //Gravidas
        criaPessoas(&(formigopolis.gravidas[0]), "Maria", priorGravida);

        //Idosos
        criaPessoas(&(formigopolis.idoso[0]), "Vanda", priorIdoso);

        //deficiente
        criaPessoas(&(formigopolis.deficiente[0]), "Paula", priorDefic);

        //comum
        criaPessoas(&(formigopolis.comum[0]), "Sueli", priorComum);
    //fim criando as pessoas

    args.formigopolis = formigopolis;


    //cria threads para cada pessoa:
    pthread_create(&gravida, NULL, pGravida, &args);
    pthread_create(&idoso, NULL, pIdosa, &args);
    pthread_create(&defic, NULL, pDefic, &args);
    pthread_create(&comum, NULL, pComum, &args);

    sleep(1);//sleep para garantir que as outras threads foram criadas e já estão esperando

    //começa o atendimento
    //pthread_create(&reordenador, NULL, reordFIFO, &args);
    pthread_create(&caixa, NULL, caixaEle, &args);

    //espera threads
    pthread_join(gravida, NULL);
    pthread_join(idoso, NULL);
    pthread_join(defic, NULL);
    pthread_join(comum, NULL);
    pthread_join(caixa, NULL);
    //pthread_join(reordenador, NULL);

    printf("\n\t!! Foram atendidas %d pessoas !! \n\n", args.atendidos);
}
//fim principal

//fim parte 1

int main () {
    Pessoa Grav[2], Idoso[2], Defic[2], Comum[2];
    lista fifoPrior;
    int vetP1[atendPart1], vetP2[atendPart2], op = -1;

    srand(time(NULL));
    
    //criando as pessoas
        //Gravidas
        criaPessoas(&(Grav[0]), "Maria", priorGravida);
        criaPessoas(&(Grav[1]), "Marcos", priorGravida);

        //Idosos
        criaPessoas(&(Idoso[0]), "Vanda", priorIdoso);
        criaPessoas(&(Idoso[1]), "Valter", priorIdoso);

        //deficiente
        criaPessoas(&(Defic[0]), "Paula", priorDefic);
        criaPessoas(&(Defic[1]), "Pedro", priorDefic);

        //comum
        criaPessoas(&(Comum[0]), "Sueli", priorComum);
        criaPessoas(&(Comum[1]), "Silas", priorComum);
    //fim criando as pessoas

    while (op != 0) {
        printf("\n\t\t!! Escolha !! \n0: Finalizar  |  1: Parte 1  |  2: Parte 2\n- ");
        scanf("%d", &op);

        switch (op)
        {
        case 0:
            break;
        case 1:
            parte1Trab();
            break;
        case 2:
            //parte2Trab();
            break;
        default:
            printf("\n\t! OPÇÃO INVÁLIDA !\n");
            break;
        }
    }

    //destruindo mutex e var de condição
    pthread_mutex_destroy(&trava_fila);
    pthread_cond_destroy(&espera_chegada);
    pthread_cond_destroy(&espera_proximo);

    return 0;
}