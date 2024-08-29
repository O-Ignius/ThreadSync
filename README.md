# Projeto TrabalhoSOCaixaEletronico

## Descrição

Este projeto é uma simulação de um sistema de atendimento em uma casa lotérica, onde várias pessoas com diferentes níveis de prioridade tentam usar o único caixa disponível. O objetivo é implementar um sistema de sincronização usando threads e técnicas de exclusão mútua para garantir que as regras de prioridade sejam respeitadas.

## Estrutura do Código

O projeto consiste no seguinte arquivo:

- `Trab2SO.c`: O código fonte principal que implementa a lógica do atendimento usando threads POSIX (`pthreads`), mutexes e variáveis de condição.

### Lógica das Filas de Prioridade

Foram utilizadas quatro filas diferentes para gerenciar as prioridades dos clientes:

1. **Fila 1**: Para clientes com prioridade mínima (por exemplo, pessoas comuns).
2. **Fila 2**: Para clientes com a segunda prioridade mais baixa (pessoas com deficiência).
3. **Fila 3**: Para clientes com a segunda maior prioridade (idosos).
4. **Fila 4**: Para clientes com prioridade máxima (grávidas ou pessoas com crianças de colo).

Clientes com prioridade maior que 4 são colocados na **Fila 4**, respeitando suas prioridades específicas. Dentro da Fila 4, clientes são ordenados com base em suas prioridades e, em caso de mesma prioridade, pela ordem de chegada.

### Implementação

- O código utiliza a biblioteca `pthread` para criar e gerenciar threads.
- São utilizados mutexes para garantir a exclusão mútua ao acessar o caixa.
- Variáveis de condição são usadas para controlar o acesso ao caixa com base nas prioridades das filas.
- O código implementa um mecanismo para evitar inanição, aumentando gradualmente a prioridade dos clientes que foram preteridos no atendimento repetidamente.

### Como Compilar e Executar

1. Certifique-se de ter o compilador `gcc` e a biblioteca `pthread` instalados no seu sistema.
2. Use o Makefile fornecido para compilar o projeto:
   ```bash
   make

