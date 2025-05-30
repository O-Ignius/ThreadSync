#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/wait.h>

#define atendPart1 4        //quantia de pessoas atendidas na parte 2
#define atendPart2 8        //quantia de pessoas atendidas na parte 2

#define medidorPaciencia 2  //constante para determinar o quanto é passivel alguem passar na sua frente na fila
#define esperaReordena 5    //constante para setar um tempo para o reordenador esperar antes de reordenar novamente
#define esperaCaixa 1       //constante para setar um tempo de espera do caixa após o atendimento
#define tempoEsperaPessoa 5 //constante para setar um tempo de espera pra pessoa após ser atendida pelo caixa

#define priorGravida 4      //prioridade base de gravida
#define priorIdoso 3        //prioridade base de idoso
#define priorDefic 2        //prioridade base de deficiente
#define priorComum 1        //prioridade base de pessoa comum

/*
Precedencias:
______________________________________________________________________________________________________________________
1° Parte

    4                     3                         2                  1
 Grávida           ->   Idoso             ->   Deficiente       ->   Comum
(Maria e Marcos)       (Vanda e Valter)       (Paula e Pedro)       (Sueli e Silas)

______________________________________________________________________________________________________________________
2° Parte 

               4                     3                         2                  1
            Grávida           ->   Idoso             ->   Deficiente       ->   Comum
        (Maria e Marcos)       (Vanda e Valter)       (Paula e Pedro)       (Sueli e Silas)
                                                            |
    ^
    |-------------------------------------------------------|
    

______________________________________________________________________________________________________________________
*/

//tipo pessoa
typedef struct
{
    char nome[20];
    int prioridade;
    int espera;
    int id;
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
    lista FIFO4, FIFO3, FIFO2, FIFO1;
    cidadaos formigopolis;
    int atendidos, esperando;
    int qntAtend, parteTrab, qntRep;
}argsT;

//fim argumentos para threads

//inicializando variaveis pthread
pthread_mutex_t trava_fila = PTHREAD_MUTEX_INITIALIZER;
static sem_t condMaria;
static sem_t  condMarcos;
static sem_t  condVanda;
static sem_t  condValter;
static sem_t  condPaula;
static sem_t  condPedro; 
static sem_t  condSueli;
static sem_t  condSilas;
//

//lidar com a lista
//verifica lista vazia | retorna 1 se vazia
int Empt (lista Lista) {
    if (Lista.first == NULL) {
        return 1;
    }
    return 0;
}

//verifica se deu erro de alocação de memória
void erroAloc(void* p) {
    if (p == NULL) {
        perror("Erro de alocação de memória! ");
        exit(1);
    }
}

//insere na fila de acordo com o valor da prioridade da pessoa
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
    //se fila tiver mais de 1 item
    else if (aux->prox != NULL) {
        //verifica se deve adicionar no inicio da fila
        if (aux->pessoa.prioridade < pessoa.prioridade) {
            auxPosi = aux;
            inseriu = 1;
            while (auxPosi != NULL)
            {
                auxPosi->pessoa.espera++;   //aumenta a espera de todos na fila
                if (auxPosi->pessoa.espera > (medidorPaciencia - 1)) {
                    aumentaPrioridade = auxPosi->pessoa.espera / medidorPaciencia;
                    auxPosi->pessoa.espera = auxPosi->pessoa.espera % medidorPaciencia;
                    auxPosi->pessoa.prioridade += aumentaPrioridade;
                }

                auxPosi = auxPosi->prox;
            }

            alocAUX = malloc(sizeof(*Lista->first));
            erroAloc(alocAUX);
            alocAUX->pessoa = pessoa;
            alocAUX->prox = aux;
            Lista->first = alocAUX;
        }
        //se não adicionar no inicio, verifica qual posição da fila deve ser adicionado
        else {
            while (aux->prox != NULL)
            {
                if (aux->prox->pessoa.prioridade < pessoa.prioridade) {
                    auxPosi = aux;
                    inseriu = 1;
                    while (auxPosi != NULL)
                    {
                        auxPosi->pessoa.espera++;   //aumenta a espera de todos na fila
                        if (auxPosi->pessoa.espera > (medidorPaciencia - 1)) {
                            aumentaPrioridade = auxPosi->pessoa.espera / medidorPaciencia;
                            auxPosi->pessoa.espera = auxPosi->pessoa.espera % medidorPaciencia;
                            auxPosi->pessoa.prioridade += aumentaPrioridade;
                        }

                        auxPosi = auxPosi->prox;
                    }

                    alocAUX = malloc(sizeof(*Lista->first));
                    erroAloc(alocAUX);
                    alocAUX->pessoa = pessoa;
                    alocAUX->prox = aux->prox;
                    aux->prox = alocAUX;
                    
                    break;
                }

                aux = aux->prox;                    //avança o ponteiro
            }
        }
        //se não adicionou, deve-se adicioná-lo ao final da fila
        if (inseriu == 0) {
            aux->prox = malloc(sizeof(*Lista->first));
            erroAloc(aux->prox);                          //verifica se funcionou
            aux->prox->pessoa = pessoa;
            aux->prox->prox = NULL;
        }
    }
    //se fila tiver somente 1 item
    else {
        if (aux->pessoa.prioridade < pessoa.prioridade) {
            alocAUX = malloc(sizeof(*Lista->first));    //aloca ponteiro auxiliar
            erroAloc(alocAUX);                          //verifica se funcionou
            alocAUX->pessoa = pessoa;                   //coloca a pessoa nesse ponteiro
            alocAUX->prox = aux;                        //aponta a proxima posição alocada para a posição atual

            //aumenta espera
            aux->pessoa.espera++;
            if (aux->pessoa.espera > (medidorPaciencia - 1)) {
                aumentaPrioridade = aux->pessoa.espera / medidorPaciencia;
                aux->pessoa.espera = aux->pessoa.espera % medidorPaciencia;
                aux->pessoa.prioridade += aumentaPrioridade;
            }
            
            Lista->first = alocAUX;                              //aponta a 1° posição atual para a posição alocada
        }
        else {
            aux->prox = malloc(sizeof(*Lista->first));
            erroAloc(aux->prox);                          //verifica se funcionou
            aux->prox->pessoa = pessoa;
            aux->prox->prox = NULL;
        }
    }
}

//remove a primeira célula da fila e a retorna
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

//remove uma celula da fila, a remoção ocorre com relação ao indice i passado, onde a célula removida é retornada para ser recolocada na fila com relação a sua prioridade
Pessoa removeFIFOPReordena(lista *Lista, int i) {
    pointer auxFIFO = Lista->first, aux2 = NULL;
    Pessoa reordenada;
    int aux = 0;

    if (Empt(*Lista) == 1) {
        printf("FIFO vazia!\n");
        exit(1);
    }
    else if (aux == i) {    //se estiver na primeira posição
        reordenada = Lista->first->pessoa;
        Lista->first = Lista->first->prox;
        free(auxFIFO);
        return (reordenada);
    }
    else {
        while (aux < (i - 1))//vai até o ponteiro anterior da posição da célula que se irá reordenar
        {
            auxFIFO = auxFIFO->prox;
            aux++;
        }

        reordenada = auxFIFO->prox->pessoa;         //pega o valor da pessoa que se ira reordenar para retorno
        aux2 = auxFIFO->prox;                       //salva ponteiro pro free
        auxFIFO->prox = auxFIFO->prox->prox;        //aponta para a célula nova próxima 
        free(aux2);                                 //libera
        return reordenada;
    }
}

//fim lidar com a lista


void criaPessoas(Pessoa *p, char *nome, int prioridade, int id) {
    strcpy(p->nome, nome);
    p->prioridade = prioridade;
    p->espera = 0;
    p->id = id;
}

//geral

//printa resultados:
void printaAtendido(char *nome) {
    printf("%s está sendo atendido(a)\n", nome);
    fflush(stdout);
}

void printaLiberandoP(char *nome) {
    printf("liberando %s para atendimento\n", nome);
    fflush(stdout);
}

void printaTaNaFila (char *nome) {
    printf("%s está na fila do caixa\n", nome);
    fflush(stdout);
}

void printaFoiPCasa(char *nome) {
    printf("%s vai para casa\n", nome);
    fflush(stdout);
}
//fim printa resultados

void travaEADDFIFO(argsT *args, Pessoa p) {
    pthread_mutex_lock(&trava_fila);                        //trava a fila

        if (p.prioridade == 1) {
            insereFilaPrior(&(args->FIFO1), p);
        }
        else if (p.prioridade == 2) {
            insereFilaPrior(&(args->FIFO2), p);
        }
        else if (p.prioridade == 3) {
            insereFilaPrior(&(args->FIFO3), p);
        }
        else {
            insereFilaPrior(&(args->FIFO4), p);
        }
        args->esperando++;
        
    pthread_mutex_unlock(&trava_fila);  
}

//tenta criar thread
void criaThread(pthread_t *valThread, void *funcao, void *args) {
    if (pthread_create(valThread, NULL, funcao, args) != 0) {
        perror("Erro de criação de thread! ");
        exit(1);
    }
}

int quantasFilas(int qual, argsT *args) {
    if (qual == 3) {
        if (Empt(args->FIFO4) != 1) {
            return 1;
        }
    }
    else if (qual == 2) {
        if (Empt(args->FIFO4) != 1 || Empt(args->FIFO3) != 1) {
            return 1;
        }
    }
    else {
        if (Empt(args->FIFO4) != 1 || Empt(args->FIFO3) != 1 || Empt(args->FIFO2) != 1) {
            return 1;
        }
    }
    return -1;
}

void qualFilaInserir(Pessoa p, argsT *args) {
    if (p.prioridade >= 4) {
        insereFilaPrior(&(args->FIFO4), p);
    }
    else if (p.prioridade > 3) {
        insereFilaPrior(&(args->FIFO3), p);
    }
    else {
        insereFilaPrior(&(args->FIFO2), p);
    }
}

//reordenar fila
void *reordFIFO(void *t) {
    argsT *args = (argsT *) t;
    pointer aux = NULL;
    Pessoa pReinsert;
    int fila1, fila2, fila3, fila4;

    while (1)
    {
        fila1 = 0; fila2 = 0; fila3 = 0; fila4 = 0;

        pthread_mutex_lock(&trava_fila);                        //trava a fila
            if (args->atendidos == args->qntAtend) {    //espera todos serem atendidos para encerrar
                pthread_mutex_unlock(&trava_fila);
                pthread_exit(NULL);                            //finaliza programa
            }
            
            //fila 4
            aux = args->FIFO4.first;
            while (aux != NULL)
            {
                if (aux != NULL && aux->prox != NULL) {
                    if (aux->prox->pessoa.prioridade > aux->pessoa.prioridade) {
                        pReinsert = removeFIFOPReordena(&(args->FIFO4), (fila4 + 1));
                        printf("Aumentando prioridade de %s \n", pReinsert.nome);
                        fflush(stdout);

                        qualFilaInserir(pReinsert, args);
                    }
                }

                if (args->FIFO4.first == NULL) {
                    break;
                }
                aux = aux->prox;
                fila4++;
            }
            
            //fila 3
            aux = args->FIFO3.first;
            while (aux != NULL)
            {
                if (aux->pessoa.prioridade > priorIdoso) {
                    if (quantasFilas(3, args) == 1) {
                        pReinsert = removeFIFOPReordena(&(args->FIFO3), (fila3));
                        printf("Aumentando prioridade de %s \n", pReinsert.nome);
                        fflush(stdout);

                        qualFilaInserir(pReinsert, args);
                        
                        fila3--;
                    }
                }

                if (args->FIFO3.first == NULL) {
                    break;
                }

                fila3++;
                aux = aux->prox;
            }

            //fila 2
            aux = args->FIFO2.first;
            while (aux != NULL)
            {
                if (aux->pessoa.prioridade > priorDefic) {
                    if (quantasFilas(2, args) == 1) {
                        pReinsert = removeFIFOPReordena(&(args->FIFO2), (fila2));
                        printf("Aumentando prioridade de %s \n", pReinsert.nome);
                        fflush(stdout);

                        qualFilaInserir(pReinsert, args);

                        fila2--;
                    }
                }

                if (args->FIFO2.first == NULL) {
                    break;
                }

                fila2++;
                aux = aux->prox;
            }

            //fila 1
            aux = args->FIFO1.first;
            while (aux != NULL)
            {
                if (aux->pessoa.prioridade > priorComum) {
                    if (quantasFilas(1, args) == 1) {
                        pReinsert = removeFIFOPReordena(&(args->FIFO1), (fila1));
                        printf("Aumentando prioridade de %s \n", pReinsert.nome);
                        fflush(stdout);

                        qualFilaInserir(pReinsert, args);

                        fila1--;
                    }
                    
                }

                if (args->FIFO1.first == NULL) {
                    break;
                }

                fila1++;
                aux = aux->prox;
            }
            
        pthread_mutex_unlock(&trava_fila);

        sleep(esperaReordena);
    }
}
//fim reordernar fila

void aumentaPrioridadeEsperando(pointer aux) {
    while (aux != NULL)
    {
        aux->pessoa.espera++;
        if (aux->pessoa.espera >= medidorPaciencia) {
            printf("Gerente detectou inanição, aumentando prioridade de %s\n", aux->pessoa.nome);
            aux->pessoa.espera = 0;
            aux->pessoa.prioridade++;
        }

        aux = aux->prox;
    }
}

void aumentaPrioridadeFila (argsT *args, int qual) {
    if (qual == 4) {
        if (Empt(args->FIFO3) != 1) {
            aumentaPrioridadeEsperando((args->FIFO3.first));
        }
        if (Empt(args->FIFO2) != 1) {
            aumentaPrioridadeEsperando((args->FIFO2.first));
        }
        if (Empt(args->FIFO1) != 1) {
            aumentaPrioridadeEsperando((args->FIFO1.first));
        }
    }
    else if (qual == 3) {
        if (Empt(args->FIFO2) != 1) {
            aumentaPrioridadeEsperando((args->FIFO2.first));
        }
        if (Empt(args->FIFO1) != 1) {
            aumentaPrioridadeEsperando((args->FIFO1.first));
        }
    }
    else {
        if (Empt(args->FIFO1) != 1) {
            aumentaPrioridadeEsperando((args->FIFO1.first));
        }
    }
}

void sinalizaAtendido (Pessoa P) {
    if (P.id == 1) {
        sem_post(&condMaria);
    }
    else if (P.id == 2) {
        sem_post(&condMarcos);
    }
    else if (P.id == 3) {
        sem_post(&condVanda);
    }
    else if (P.id == 4) {
        sem_post(&condValter);
    }
    else if (P.id == 5) {
        sem_post(&condPaula);
    }
    else if (P.id == 6) {
        sem_post(&condPedro);
    }
    else if (P.id == 7) {
        sem_post(&condSueli);
    }
    else if (P.id == 8) {
        sem_post(&condSilas);
    }
}

//thread caixa
void* caixaEle(void *t) {
    argsT *args = (argsT *) t;
    Pessoa atendido;
    int sort;

    while (1)
    {
        sleep(esperaCaixa);
        pthread_mutex_lock(&trava_fila);                        //trava a fila
            if (args->atendidos == args->qntAtend) {//condição de parada
                pthread_mutex_unlock(&trava_fila);
                pthread_exit(NULL);                            //finaliza programa
            }
            
            if ((Empt(args->FIFO4) != 1) && (Empt(args->FIFO3) != 1) && (Empt(args->FIFO2) != 1)) {    //há deadlock, vamo esperar um sorteio
                printf("Gerente detectou deadlock, ");
                
                sort = ((rand() % 5) + 2);
                if (sort == 2) {                    //libera o deficiente
                    printaLiberandoP(args->FIFO2.first->pessoa.nome);
                    printaAtendido(args->FIFO2.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO2));
                    printaFoiPCasa(atendido.nome);  

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 2);
                }
                else if (sort == 3) {                        //libera o idoso
                    printaLiberandoP(args->FIFO3.first->pessoa.nome);
                    printaAtendido(args->FIFO3.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO3));
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 3);
                }
                else {                              //libera a gravida
                    printaLiberandoP(args->FIFO4.first->pessoa.nome);
                    printaAtendido(args->FIFO4.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO4));
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 4);
                }

                //USADO SOMENTE PARA DEPURAÇÃO | FIRULAS
                args->atendidos++;
                args->esperando--;
            }
            else {
                if (Empt(args->FIFO4) != 1) {
                    printaAtendido(args->FIFO4.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO4));
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 4);
                }
                else if (Empt(args->FIFO3) != 1) {
                    printaAtendido(args->FIFO3.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO3)); 
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 3);
                }
                else if (Empt(args->FIFO2) != 1) {
                    printaAtendido(args->FIFO2.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO2)); 
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);

                    aumentaPrioridadeFila(args, 2);
                }
                else if (Empt(args->FIFO1) != 1) {
                    printaAtendido(args->FIFO1.first->pessoa.nome);   //printa quem está sendo atendido
                    atendido = removeFIFO(&(args->FIFO1)); 
                    printaFoiPCasa(atendido.nome);

                    sinalizaAtendido(atendido);
                }

                //USADO SOMENTE PARA DEPURAÇÃO | FIRULAS
                args->atendidos++;
                args->esperando--;
            }
        pthread_mutex_unlock(&trava_fila);                              //libera fila/
    
        sleep(esperaCaixa);
    }
}
//fim thread caixa

//thread clientes
void* pGravida(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.gravidas[0]);
        printaTaNaFila(args->formigopolis.gravidas[0].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condMaria);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }

    pthread_exit(NULL);                            //finaliza programa
}

void *pIdosa(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.idoso[0]);
        printaTaNaFila(args->formigopolis.idoso[0].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condVanda);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }
    
    pthread_exit(NULL);                            //finaliza programa
}

void *pDefic(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.deficiente[0]);
        printaTaNaFila(args->formigopolis.deficiente[0].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condPaula);

            sleep(((rand() % 3) + 3));    
        }

        i++;
    }

    pthread_exit(NULL);                            //finaliza programa
}

void *pComum(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.comum[0]);
        printaTaNaFila(args->formigopolis.comum[0].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condSueli);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }

    pthread_exit(NULL);                            //finaliza programa
}
//fim thread clientes

//parte 2 trabalho
void* pGravida2(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.gravidas[1]);
        printaTaNaFila(args->formigopolis.gravidas[1].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condMarcos);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }
    
    pthread_exit(NULL);                            //finaliza programa
}

void *pIdosa2(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.idoso[1]);
        printaTaNaFila(args->formigopolis.idoso[1].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condValter);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }
    
    pthread_exit(NULL);                            //finaliza programa
}

void *pDefic2(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.deficiente[1]);
        printaTaNaFila(args->formigopolis.deficiente[1].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condPedro);
        
            sleep(((rand() % 3) + 3));
        }

        i++;
    }
      
    pthread_exit(NULL);                            //finaliza programa
}

void *pComum2(void *t) {
    argsT *args = (argsT *) t;
    int i = 0;

    while (i < args->qntRep)
    {
        travaEADDFIFO(args, args->formigopolis.comum[1]);
        printaTaNaFila(args->formigopolis.comum[1].nome);

        if (i < args->qntRep - 1) {
            sem_wait(&condSilas);

            sleep(((rand() % 3) + 3));
        }
        
        i++;
    }
    pthread_exit(NULL);                            //finaliza programa
}


//principal
void parte2Trab(cidadaos formigopolis, int qntRep) {
    pthread_t gravida[(qntRep * 2)], idoso[(qntRep * 2)], defic[(qntRep * 2)], comum[(qntRep * 2)], caixa, reordenador;
    argsT args;
    int i = 0;

    args.atendidos = 0; args.esperando = 0; args.qntAtend = (atendPart2 * qntRep);
    args.formigopolis = formigopolis; args.parteTrab = 2; args.qntRep = qntRep;
  
    if (qntRep > 0) {
        //cria threads para cada pessoa:
        criaThread(&(gravida[i]), pGravida, &args);
        criaThread(&(idoso[i]), pIdosa, &args);
        criaThread(&(defic[i]), pDefic, &args);
        criaThread(&(comum[i]), pComum, &args);

        criaThread(&(gravida[i]), pGravida2, &args);
        criaThread(&(idoso[i]), pIdosa2, &args);
        criaThread(&(defic[i]), pDefic2, &args);
        criaThread(&(comum[i]), pComum2, &args);

        //começa o atendimento
        pthread_create(&reordenador, NULL, reordFIFO, &args);
        pthread_create(&caixa, NULL, caixaEle, &args);
    }

    if (qntRep > 0) {
        pthread_join(caixa, NULL);
        pthread_join(reordenador, NULL);
    }
    
    //printf("\n\t!! Foram atendidas %d pessoas !! \n\n", args.atendidos);
}
//fim principal
//fim parte 2

int main (int argc, char const *argv[]) {
    cidadaos formigopolis;
    int qntRep = atoi(argv[1]);

    srand(time(NULL));
    
    //criando as pessoas
        //Gravidas
        criaPessoas(&(formigopolis.gravidas[0]), "Maria", priorGravida, 1);
        criaPessoas(&(formigopolis.gravidas[1]), "Marcos", priorGravida, 2);
        
        //Idosos
        criaPessoas(&(formigopolis.idoso[0]), "Vanda", priorIdoso, 3);
        criaPessoas(&(formigopolis.idoso[1]), "Valter", priorIdoso, 4);

        //deficiente
        criaPessoas(&(formigopolis.deficiente[0]), "Paula", priorDefic, 5);
        criaPessoas(&(formigopolis.deficiente[1]), "Pedro", priorDefic, 6);

        //comum
        criaPessoas(&(formigopolis.comum[0]), "Sueli", priorComum, 7);
        criaPessoas(&(formigopolis.comum[1]), "Silas", priorComum, 8);
    //fim criando as pessoas

    //inicia semaforos
    sem_init(&condMaria, 0, 0);
    sem_init(&condMarcos, 0, 0);
    sem_init(&condVanda, 0, 0);
    sem_init(&condValter, 0, 0);
    sem_init(&condPaula, 0, 0);
    sem_init(&condPedro, 0, 0);
    sem_init(&condSueli, 0, 0);
    sem_init(&condSilas, 0, 0);
    //fim semaforos

    parte2Trab(formigopolis, qntRep);

    //destruindo mutex e var de condição
    pthread_mutex_destroy(&trava_fila);
    sem_destroy(&condMaria);
    sem_destroy(&condMarcos);
    sem_destroy(&condVanda);
    sem_destroy(&condValter);
    sem_destroy(&condPaula);
    sem_destroy(&condPedro);
    sem_destroy(&condSueli);
    sem_destroy(&condSilas);
    return 0;
}