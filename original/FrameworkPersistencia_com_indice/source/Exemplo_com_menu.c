#include <stdio.h>
#include <strings.h>
#include "Utils.h"
#include "Persistencia.h"

struct Pessoa{
   int          cpf;
   char         nome[30];
   int          idade;
};

//---------------------------------------------
int comparaChavePessoa(void *info1, void *info2){
  int           *chave = (int *) info1;
  struct Pessoa *p     = (struct Pessoa*) info2;

  return *chave - p->cpf;
}

//---------------------------------------------
// funcoes exigidas pelo indice (arvore B+) mantido por abrir():
// uma para extrair a chave (cpf) de dentro de um registro, e
// outra para comparar duas chaves (cpf) entre si.
void* extraiChaveCpf(void* dados){
    struct Pessoa *p = (struct Pessoa*) dados;
    return &(p->cpf);
}

int comparaChaveCpf(void* chave1, void* chave2){
    int *c1 = (int*) chave1;
    int *c2 = (int*) chave2;
    return *c1 - *c2;
}

//---------------------------------------------
void imprimePessoa(void *dados){

    struct Pessoa *pp = (struct Pessoa *) dados;
    printf("CPF: %d - Nome: %s - Idade: %d \n", pp->cpf, pp->nome, pp->idade);
}


int   maiorDeIdade (void * info){
    struct Pessoa *p = (struct Pessoa*) info;
    if(p->idade >= 18)
        return 1;

    return 0;
}

// -------------------------------------------
// -------------------------------------------
void main(){

  struct Pessoa  eu;
  struct Pessoa  *pessoa;

  int cpf;

  int op;
  do{
    system("cls"); // Limpa a tela
    printf("1 - Cadastrar pessoa \n");
    printf("2 - Listar pessoas \n");
    printf("3 - Alterar dados de uma pessoa \n");
    printf("4 - Excluir pessoa \n");
    printf("5 - Buscar pessoa \n");
    printf("6 - Listar pessoas maiores de 18 anos \n");
    printf("    ---------------------------------------------- \n");
    printf("9 - Sair \n");

    printf("Escolha uma opcao: ");
    scanf("%d", &op);
    fflush(stdin);

    pDFile arqPessoa;

    switch (op){
      case 1 :

            printf("\nCPF:");
            scanf("%d", &eu.cpf);
            fflush(stdin);
            printf("\nNome:");
            gets(&eu.nome);
            fflush(stdin);
            printf("\nIdade:");
            scanf("%d", &eu.idade);

            arqPessoa = abrir("CadastroPessoas.dat",
                                sizeof(struct Pessoa), sizeof(int),
                                extraiChaveCpf, comparaChaveCpf);

            createe(arqPessoa, &eu);
            fechar(arqPessoa);
            break;

      case 2 :
          // mostrar todas as pessoas cadastradas no arquivo
           arqPessoa = abrir("CadastroPessoas.dat",
                                sizeof(struct Pessoa), sizeof(int),
                                extraiChaveCpf, comparaChaveCpf);

          pDLista pessoas = queryAll(arqPessoa);
          imprimirLista(pessoas, imprimePessoa);


         fechar(arqPessoa);
         getch();
         break;

      case 4 :
            printf("\nDigite o CPF a ser excluido:");
            scanf("%d", &eu.cpf);

            arqPessoa = abrir("CadastroPessoas.dat",
                                sizeof(struct Pessoa), sizeof(int),
                                extraiChaveCpf, comparaChaveCpf);

            deletee(arqPessoa, &eu.cpf, comparaChavePessoa);

            fechar(arqPessoa);
            getch();
            break;

      case 3 :
          printf("\nCPF:");
          scanf("%d", &eu.cpf);
          fflush(stdin);
          printf("\nNovo nome:");
          gets(&eu.nome);
          fflush(stdin);
          printf("\nNova idade:");
          scanf("%d", &eu.idade);

          arqPessoa = abrir("CadastroPessoas.dat",
                            sizeof(struct Pessoa), sizeof(int),
                            extraiChaveCpf, comparaChaveCpf);
          update(arqPessoa, &eu.cpf, &eu, comparaChavePessoa);

          fechar(arqPessoa);
          getch();
          break;

      case 5 :
            printf("\nDigite o CPF a ser consultado:");
            scanf("%d", &eu.cpf);

            arqPessoa = abrir("CadastroPessoas.dat",
                                sizeof(struct Pessoa), sizeof(int),
                                extraiChaveCpf, comparaChaveCpf);

            pessoa = retrieve(arqPessoa, &eu.cpf,
                              comparaChavePessoa);
            if(pessoa != NULL)
                imprimePessoa(pessoa);
            else
                printf("CPF n�o cadastrado!");

            fechar(arqPessoa);
            getch();

            break;

        case 6 :
            arqPessoa = abrir("CadastroPessoas.dat",
                                sizeof(struct Pessoa), sizeof(int),
                                extraiChaveCpf, comparaChaveCpf);
            pDLista  maiores = queryBy(arqPessoa, maiorDeIdade);
            imprimirLista(maiores, imprimePessoa);

            fechar(arqPessoa);
            getch();
            break;

   }

  } while(op != 9);






}
