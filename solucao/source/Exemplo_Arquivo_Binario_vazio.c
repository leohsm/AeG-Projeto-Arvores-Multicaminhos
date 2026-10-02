#include <stdio.h>
#include "Persistencia.h"

/* ----------------------------- */
struct Pessoa{
   int          cpf;
   char         nome[30];
   int          idade;
};

void imprimePessoa(void* dados){

   struct Pessoa* pessoa = (struct Pessoa*)dados;
   printf("\n CPF: %d - Nome: %s - Idade: %d", pessoa->cpf,
                                               pessoa->nome,
                                               pessoa->idade);

}

/* ----------------------------- */
void main(){

     struct Pessoa eu;
     printf("\nCPF:");
     scanf("%d", &eu.cpf);
     fflush(stdin);
     printf("\nNome:");
     gets(&eu.nome);
     fflush(stdin);
     printf("\nIdade:");
     scanf("%d", &eu.idade);

     pDFile arqPessoa;

     arqPessoa = abrir("CadastroPessoas.dat",
                        sizeof(struct Pessoa));

     createe(arqPessoa, &eu);

     // mostrar todas as pessoas cadastradas no arquivo
     pDLista pessoas = queryAll(arqPessoa);
     imprimirLista(pessoas, imprimePessoa);

     fechar(arqPessoa);
}

