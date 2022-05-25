#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/* MARK NAME Bruno Ferreira Da Silva */
/* MARK NAME Gabriel de Araújo Marques */
/* MARK NAME E Etc */

/*****************************************************************
 * RELATÓRIO

1. Termo de compromisso

Os membros do grupo afirmam que todo o código desenvolvido para este
trabalho é de autoria própria.  Exceto pelo material listado no item 3
deste relatório, os membros do grupo afirmam não ter copiado
material da Internet nem obtiveram código de terceiros.

2. Membros do grupo e alocação de esforço

Preencha as linhas abaixo com o nome e o e-mail dos integrantes do
grupo.  Substitua XX pela contribuição de cada membro do grupo no
desenvolvimento do trabalho.

Bruno Ferreira da Silva <brunosife@gmail.com.br> 50%
Gabriel de Araújo Marques <amarques.gabriel@gmail.com> 50%

3. Referências bibliográficas

https://linux.die.net/man/2/pipe
https://linux.die.net/man/2/close
https://linux.die.net/man/2/fork
https://linux.die.net/man/3/exec

4. Estruturas de dados

Task 1: Deixamos no código um comentário respondendo à questão

Task 2: Para a task 2, ao usarmos o comando execpv para executarmos o comando passado, recebendo os argumentos necessários a ela, quando presentes.
Caso ocorra uma interrupção durante a execução lançamos uma mensagem de erro para o usuário.

Task 3: Na task 3, fazemos uma chamada do close() para fechar o stdin ou stdout (de acordo com o operador ">" ou "<") e então chamamos o open() para abrir o arquivo selecionado usando o file descriptor anteriormente fechado, redirecionando o conteúdo do arquivo para o stdin ou o conteúdo do stdout para o arquivo.

Task 4: Na task 4, fazemos uma chamada do pipe() para abrir um pipe, retornando um file descriptor para leitura e um para escrita. Então, executamos o comando à esquerda do operador "|", fechando o stdout e mandando a saída do programa para o pipe e depois executamos o programa à direita do operador "|" redirecionando os dados do pipe para o stdin. Então chamamos wait(NULL) para aguardar até os processos filhos terminarem a sua execução
*******************************************************************/

/****************************************************************
 * Shell xv6 simplificado
 *
 * Este codigo foi adaptado do codigo do UNIX xv6 e do material do
 * curso de sistemas operacionais do MIT (6.828).
 ***************************************************************/

#define MAXARGS 10

/* Todos comandos tem um tipo.  Depois de olhar para o tipo do
 * comando, o código converte um *cmd para o tipo específico de
 * comando. */
struct cmd {
  int type; /* ' ' (exec)
               '|' (pipe)
               '<' or '>' (redirection) */
};

struct execcmd {
  int type;              // ' '
  char *argv[MAXARGS];   // argumentos do comando a ser exec'utado
};

struct redircmd {
  int type;          // < ou > 
  struct cmd *cmd;   // o comando a rodar (ex.: um execcmd)
  char *file;        // o arquivo de entrada ou saída
  int mode;          // o modo no qual o arquivo deve ser aberto
  int fd;            // o número de descritor de arquivo que deve ser usado
};

struct pipecmd {
  int type;          // |
  struct cmd *left;  // lado esquerdo do pipe
  struct cmd *right; // lado direito do pipe
};

int fork1(void);  // Fork mas fechar se ocorrer erro.
struct cmd *parsecmd(char*); // Processar o linha de comando.

void runcmd(struct cmd *cmd);

void execSimpleCommand(char** argv)
{
    if (execvp(argv[0], argv) < 0) {
        printf("\nNão foi possível executar o comando.");
    }
}

void execRedirectCommand(struct redircmd *rcmd)
{
    close(rcmd->fd);
    if(open(rcmd->file, rcmd->mode) < 0){
        printf("Não é possível abrir o arquivo\n");
        return;
    }
    
    runcmd(rcmd->cmd);
}

void execPipeCommand(struct pipecmd* pcmd) {
    int p[2];

    if (pipe(p) < 0) {
        printf("\nErro ao executar pipe");
        return;
    }

    pid_t pid;

    pid = fork1();
    if (pid == 0) {
        close(STDOUT_FILENO);
        dup(p[1]);
        close(p[0]);
        close(p[1]);
        runcmd(pcmd->left);
    }

    pid = fork1();
    if (pid == 0) {
        close(STDIN_FILENO);
        dup(p[0]);
        close(p[0]);
        close(p[1]);
        runcmd(pcmd->right);
    }

    close(p[0]);
    close(p[1]);
    wait(NULL);
    wait(NULL);
}

/* Executar comando cmd.  Nunca retorna. */
void
runcmd(struct cmd *cmd)
{
  struct execcmd *ecmd;
  struct pipecmd *pcmd;
  struct redircmd *rcmd;

  if(cmd == 0)
    exit(0);
  
  switch(cmd->type){
  default:
    fprintf(stderr, "tipo de comando desconhecido\n");
    exit(-1);

  case ' ':
    ecmd = (struct execcmd*)cmd;
    if(ecmd->argv[0] == 0)
      exit(0);
    /* MARK START task2
     * TAREFA2: Implemente codigo abaixo para executar
     * comandos simples. */
    execSimpleCommand(ecmd->argv);
    /* MARK END task2 */
    break;

  case '>':
  case '<':
    rcmd = (struct redircmd*)cmd;
    /* MARK START task3
     * TAREFA3: Implemente codigo abaixo para executar
     * comando com redirecionamento. */
    execRedirectCommand(rcmd);
    /* MARK END task3 */
    break;

  case '|':
    pcmd = (struct pipecmd*)cmd;
    /* MARK START task4
     * TAREFA4: Implemente codigo abaixo para executar
     * comando com pipes. */
    execPipeCommand(pcmd);
    /* MARK END task4 */
    break;
  }    
  exit(0);
}

int
getcmd(char *buf, int nbuf)
{
  if (isatty(fileno(stdin)))
    fprintf(stdout, "$ ");
  memset(buf, 0, nbuf);
  fgets(buf, nbuf, stdin);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

int
main(void)
{
  static char buf[100];
  int r;

  // Ler e rodar comandos.
  while(getcmd(buf, sizeof(buf)) >= 0){
    /* MARK START task1 */
    /* TAREFA1: O que faz o if abaixo e por que ele é necessário?
     * Insira sua resposta no código e modifique o fprintf abaixo
     * para reportar o erro corretamente. 
     * 
     * Resposta : O IF [if(chdir(buf+3) < 0)] avalia se o prosseso foi capaz de encontrar
     * o folder informado como argumento para o comando CD, ele é necessário
     * para tratar caso não seja achado o folder informado, e não terminar a execução
     * do SO, e sim tratar o erro e retorna-lo para o usuário de uma maneira mais 
     * user friendly. O IF [if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' ')] avalia separadamente 
     * o comando CD pois ele não utiliza o execvp(), e sim o chdir() para procurar e executar o 
     * comando.*/
    if(buf[0] == 'c' && buf[1] == 'd' && buf[2] == ' '){
      buf[strlen(buf)-1] = 0;
      if(chdir(buf+3) < 0)
        fprintf(stderr, "Erro ao tentar encontrar o folder informado nos argumentos para o comando CD.\n");
      continue;
    }
    /* MARK END task1 */

    if(fork1() == 0)
      runcmd(parsecmd(buf));
    wait(&r);
  }
  exit(0);
}

int
fork1(void)
{
  int pid;
  
  pid = fork();
  if(pid == -1)
    perror("fork");
  return pid;
}

/****************************************************************
 * Funcoes auxiliares para criar estruturas de comando
 ***************************************************************/

struct cmd*
execcmd(void)
{
  struct execcmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = ' ';
  return (struct cmd*)cmd;
}

struct cmd*
redircmd(struct cmd *subcmd, char *file, int type)
{
  struct redircmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = type;
  cmd->cmd = subcmd;
  cmd->file = file;
  cmd->mode = (type == '<') ?  O_RDONLY : O_WRONLY|O_CREAT|O_TRUNC;
  cmd->fd = (type == '<') ? 0 : 1;
  return (struct cmd*)cmd;
}

struct cmd*
pipecmd(struct cmd *left, struct cmd *right)
{
  struct pipecmd *cmd;

  cmd = malloc(sizeof(*cmd));
  memset(cmd, 0, sizeof(*cmd));
  cmd->type = '|';
  cmd->left = left;
  cmd->right = right;
  return (struct cmd*)cmd;
}

/****************************************************************
 * Processamento da linha de comando
 ***************************************************************/

char whitespace[] = " \t\r\n\v";
char symbols[] = "<|>";

int
gettoken(char **ps, char *es, char **q, char **eq)
{
  char *s;
  int ret;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  if(q)
    *q = s;
  ret = *s;
  switch(*s){
  case 0:
    break;
  case '|':
  case '<':
    s++;
    break;
  case '>':
    s++;
    break;
  default:
    ret = 'a';
    while(s < es && !strchr(whitespace, *s) && !strchr(symbols, *s))
      s++;
    break;
  }
  if(eq)
    *eq = s;
  
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return ret;
}

int
peek(char **ps, char *es, char *toks)
{
  char *s;
  
  s = *ps;
  while(s < es && strchr(whitespace, *s))
    s++;
  *ps = s;
  return *s && strchr(toks, *s);
}

struct cmd *parseline(char**, char*);
struct cmd *parsepipe(char**, char*);
struct cmd *parseexec(char**, char*);

/* Copiar os caracteres no buffer de entrada, comeando de s ate es.
 * Colocar terminador zero no final para obter um string valido. */
char 
*mkcopy(char *s, char *es)
{
  int n = es - s;
  char *c = malloc(n+1);
  assert(c);
  strncpy(c, s, n);
  c[n] = 0;
  return c;
}

struct cmd*
parsecmd(char *s)
{
  char *es;
  struct cmd *cmd;

  es = s + strlen(s);
  cmd = parseline(&s, es);
  peek(&s, es, "");
  if(s != es){
    fprintf(stderr, "leftovers: %s\n", s);
    exit(-1);
  }
  return cmd;
}

struct cmd*
parseline(char **ps, char *es)
{
  struct cmd *cmd;
  cmd = parsepipe(ps, es);
  return cmd;
}

struct cmd*
parsepipe(char **ps, char *es)
{
  struct cmd *cmd;

  cmd = parseexec(ps, es);
  if(peek(ps, es, "|")){
    gettoken(ps, es, 0, 0);
    cmd = pipecmd(cmd, parsepipe(ps, es));
  }
  return cmd;
}

struct cmd*
parseredirs(struct cmd *cmd, char **ps, char *es)
{
  int tok;
  char *q, *eq;

  while(peek(ps, es, "<>")){
    tok = gettoken(ps, es, 0, 0);
    if(gettoken(ps, es, &q, &eq) != 'a') {
      fprintf(stderr, "missing file for redirection\n");
      exit(-1);
    }
    switch(tok){
    case '<':
      cmd = redircmd(cmd, mkcopy(q, eq), '<');
      break;
    case '>':
      cmd = redircmd(cmd, mkcopy(q, eq), '>');
      break;
    }
  }
  return cmd;
}

struct cmd*
parseexec(char **ps, char *es)
{
  char *q, *eq;
  int tok, argc;
  struct execcmd *cmd;
  struct cmd *ret;
  
  ret = execcmd();
  cmd = (struct execcmd*)ret;

  argc = 0;
  ret = parseredirs(ret, ps, es);
  while(!peek(ps, es, "|")){
    if((tok=gettoken(ps, es, &q, &eq)) == 0)
      break;
    if(tok != 'a') {
      fprintf(stderr, "syntax error\n");
      exit(-1);
    }
    cmd->argv[argc] = mkcopy(q, eq);
    argc++;
    if(argc >= MAXARGS) {
      fprintf(stderr, "too many args\n");
      exit(-1);
    }
    ret = parseredirs(ret, ps, es);
  }
  cmd->argv[argc] = 0;
  return ret;
}

// vim: expandtab:ts=2:sw=2:sts=2

