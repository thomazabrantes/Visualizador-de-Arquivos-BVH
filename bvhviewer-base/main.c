// **********************************************************************
//	BVHViewer.c
//  Desenha e anima um esqueleto a partir de um arquivo BVH (BioVision)
//  Marcelo Cohen
//  marcelo.cohen@pucrs.br
// **********************************************************************

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "opengl.h"

// Raiz da hierarquia
Node *root;

float **data = NULL;
int totalFrames = 0;
int totalChannels = 0; 
int frameCount = 0;
int frameDataSize = 0;
float *motionData = 0;

#define MAX_LINE_LENGTH 256
#define MAX_NAME_LENGTH 128

// Frame atual
int curFrame = 0;

// Funcoes para liberacao de memoria da hierarquia
void freeTree();
void freeNode(Node *node);
void parseMotion(FILE *file);

// Funcao externa para inicializacao da OpenGL
void init();

// Funcao de teste para criar um esqueleto inicial
void initMaleSkel();

// Funcoes para criacao de nodos e aplicacao dos valores de um frame
Node *createNode(char *name, Node *parent, int numChannels, float ofx,
                 float ofy, float ofz);
void applyData(float data[], Node *n);
void apply();

// **********************************************************************
//  Cria um nodo novo para a hierarquia, fazendo também a ligacao com
//  o seu pai (se houver)
//  Parametros:
//  - name: string com o nome do nodo
//  - parent: ponteiro para o nodo pai (NULL se for a raiz)
//  - numChannels: quantidade de canais de transformacao (3 ou 6)
//  - ofx, ofy, ofz: offset (deslocamento) lido do arquivo
// **********************************************************************

//
// DADOS DE EXEMPLO DO PRIMEIRO FRAME
//

Node *createNode(char *name, Node *parent, int numChannels, float ofx,
                 float ofy, float ofz) {
  Node *aux = malloc(sizeof(Node));
  aux->channels = numChannels;
  aux->channelData = calloc(sizeof(float), numChannels);
  strcpy(aux->name, name);
  aux->offset[0] = ofx;
  aux->offset[1] = ofy;
  aux->offset[2] = ofz;
  aux->numChildren = 0;
  aux->children = NULL;
  aux->parent = parent;
  aux->next = NULL;
  if (parent) {
    if(parent->children == NULL) {
      // printf("First child: %s\n", aux->name);
      parent->children = aux;
    }
    else {
      Node* ptr = parent->children;
      while(ptr->next != NULL)
        ptr = ptr->next;
      // printf("Next child: %s\n", aux->name);
      ptr->next = aux;
    }
    parent->numChildren++;
  }
  printf("Created %s\n", name);
  return aux;
}

void trimString(char *str) {
    if (!str) return;
    
    // Remove espaços do início
    char *start = str;
    while (*start && (*start == ' ' || *start == '\t')) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }
    
    // Remove espaços e quebras de linha do final
    char *end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\n' || *end == '\r' || *end == '\t')) {
        *end = '\0';
        end--;
    }
}

void parseHierarchy(FILE *file) {
    if (!file) {
        printf("Erro: arquivo inválido\n");
        return;
    }

    char line[MAX_LINE_LENGTH];
    char name[MAX_NAME_LENGTH];
    Node *currentNode = NULL;
    int lineNumber = 0;
    
    while (fgets(line, sizeof(line), file)) {
        lineNumber++;
        trimString(line);  // Limpa a linha inteira antes de processá-la
        
        if (strlen(line) == 0) {
            continue;  // Pula linhas vazias após o trim
        }
        
        printf("\nProcessando linha %d: '%s'\n", lineNumber, line);
        
        if (strncmp(line, "ROOT", 4) == 0) {
            char *nameStart = line + 4;  // Pula "ROOT"
            trimString(nameStart);       // Limpa espaços extras antes do nome
            if (sscanf(nameStart, "%s", name) == 1) {
                root = createNode(name, NULL, 0, 0, 0, 0);
                currentNode = root;
                printf("ROOT criado: %s\n", name);
            }
        }
        else if (strncmp(line, "JOINT", 5) == 0) {
            char *nameStart = line + 5;  // Pula "JOINT"
            trimString(nameStart);       // Limpa espaços extras antes do nome
            if (sscanf(nameStart, "%s", name) == 1) {
                Node *newNode = createNode(name, currentNode, 0, 0, 0, 0);
                currentNode = newNode;
                printf("JOINT criado: %s (pai: %s)\n", name, 
                       currentNode->parent ? currentNode->parent->name : "NULL");
            }
        }
        else if (strncmp(line, "OFFSET", 6) == 0) {
            char *offsetValues = line + 6;  // Pula "OFFSET"
            trimString(offsetValues);       // Limpa espaços extras antes dos valores
            float x, y, z;
            if (currentNode && sscanf(offsetValues, "%f %f %f", &x, &y, &z) == 3) {
                currentNode->offset[0] = x;
                currentNode->offset[1] = y;
                currentNode->offset[2] = z;
                printf("OFFSET definido para %s: %.2f %.2f %.2f\n", 
                       currentNode->name, x, y, z);
            }
        }
        else if (strncmp(line, "CHANNELS", 8) == 0) {
            char *channelInfo = line + 8;  // Pula "CHANNELS"
            trimString(channelInfo);       // Limpa espaços extras antes do número
            if (currentNode) {
                int numChannels;
                if (sscanf(channelInfo, "%d", &numChannels) == 1) {
                    currentNode->channels = numChannels;
                    currentNode->channelData = calloc(numChannels, sizeof(float));
                    if (currentNode->channelData) {
                        totalChannels += numChannels;
                        printf("CHANNELS configurado para %s: %d\n", 
                               currentNode->name, numChannels);
                    }
                }
            }
        }
        else if (strncmp(line, "End Site", 8) == 0) {
            if (currentNode) {
                printf("Processando End Site para nó %s\n", currentNode->name);
                // Lê a próxima linha que deve ser "{"
                if (fgets(line, sizeof(line), file)) {
                    trimString(line);
                    if (line[0] == '{') {
                        // Lê a linha do OFFSET
                        if (fgets(line, sizeof(line), file)) {
                            trimString(line);
                            float x, y, z;
                            if (sscanf(line, "OFFSET %f %f %f", &x, &y, &z) == 3) {
                                createNode("End Site", currentNode, 0, x, y, z);
                            }
                        }
                        // Lê o "}" final
                        fgets(line, sizeof(line), file);
                        trimString(line);
                    }
                }
            }
        }
        else if (strcmp(line, "}") == 0) {
            if (currentNode && currentNode->parent) {
                printf("Voltando do nó %s para o pai %s\n", 
                       currentNode->name, 
                       currentNode->parent->name);
                currentNode = currentNode->parent;
            }
        }
        else if (strcmp(line, "{") == 0) {
            printf("Início de bloco encontrado\n");
        }
        else if (strncmp(line, "MOTION", 6) == 0) {
            printf("MOTION encontrado - iniciando parsing dos frames\n");
            parseMotion(file);
        }
        else {
            printf("Aviso: linha não reconhecida: '%s'\n", line);
        }
    }
    
    printf("\nParsing da hierarquia concluído. Total de linhas processadas: %d\n", lineNumber);
}

void parseMotion(FILE *file) {
    char line[1024]; // Buffer para leitura de linhas

    // Lê a linha com "Frames: X"
    if (fgets(line, sizeof(line), file)) {
        trimString(line);
        if (strncmp(line, "Frames:", 7) == 0) {
            sscanf(line, "Frames: %d", &totalFrames);
            printf("Total de frames: %d\n", totalFrames);
        } else {
            printf("Erro: Formato inesperado. Esperado 'Frames:'.\n");
            return;
        }
    }

    // Lê e ignora a linha "Frame Time: X"
    if (fgets(line, sizeof(line), file)) {
        trimString(line);
        if (strncmp(line, "Frame Time:", 11) != 0) {
            printf("Erro: Formato inesperado. Esperado 'Frame Time:'.\n");
            return;
        }
        printf("Frame Time ignorado: %s\n", line);
    }

    // Aloca memória para a matriz de dados
    data = (float **)malloc(totalFrames * sizeof(float *));
    if (!data) {
        printf("Erro: Falha ao alocar memória para as linhas da matriz.\n");
        exit(1);
    }

    for (int i = 0; i < totalFrames; i++) {
        data[i] = NULL; // Inicializa os ponteiros
    }

    // Variáveis de controle
    int currentFrame = 0;

    // Lê os dados de movimento
    while (fgets(line, sizeof(line), file)) {
        trimString(line);

        // Divide a linha em tokens (valores de movimento)
        char *token = strtok(line, " ");
        int channelIndex = 0;

        if (currentFrame == 0) {
            // Conta o número de canais apenas na primeira linha
            while (token != NULL) {
                channelIndex++;
                token = strtok(NULL, " ");
            }
            totalChannels = channelIndex;

            // Aloca memória para as colunas
            for (int i = 0; i < totalFrames; i++) {
                data[i] = (float *)malloc(totalChannels * sizeof(float));
                if (!data[i]) {
                    printf("Erro: Falha ao alocar memória para as colunas da matriz.\n");
                    exit(1);
                }
            }
            printf("Total de canais: %d\n", totalChannels);

            // Reinicia o token para processar a primeira linha novamente
            token = strtok(line, " ");
            channelIndex = 0;
        }

        // Processa a linha atual
        while (token != NULL) {
            if (channelIndex < totalChannels) {
                data[currentFrame][channelIndex] = atof(token);
                channelIndex++;
            }
            token = strtok(NULL, " ");
        }

        if (channelIndex != totalChannels) {
            printf("Aviso: Dados incompletos no frame %d. Esperados: %d, Lidos: %d\n", currentFrame, totalChannels, channelIndex);
        }

        currentFrame++;
    }

    if (currentFrame != totalFrames) {
        printf("Aviso: Número de frames lidos (%d) não corresponde ao total esperado (%d).\n", currentFrame, totalFrames);
    }

    printf("Dados de movimento carregados com sucesso.\n");
}

void freeMotionData() {
    if (data) {
        for (int i = 0; i < totalFrames; i++) {
            free(data[i]);
        }
        free(data);
        data = NULL;
    }
}

void printHierarchy(Node *node, int depth) {
    if (!node) return;

    for (int i = 0; i < depth; i++) printf("  "); // Indentação
    printf("%s (Canais: %d, Filhos: %d)\n", node->name, node->channels, node->numChildren);

    Node *child = node->children;
    while (child) {
        printHierarchy(child, depth + 1);
        child = child->next;
    }
}

// Pos. da aplicacao dos dados
int dataPos;

void applyData(float data[], Node *n) {
  // printf("%s:\n", n->name);
  if (n->numChildren == 0)
    return;
  for (int c = 0; c < n->channels; c++) {
    // printf("   %d -> %f\n", c, data[dataPos]);
    n->channelData[c] = data[dataPos++];
  }
  Node* ptr = n->children;
  while(ptr != NULL) {
    applyData(data, ptr);
    ptr = ptr->next;
  }
}

void apply() {
  dataPos = 0;
  applyData(data[curFrame], root);
}

void initMaleSkel() {
  root = createNode("Hips", NULL, 6, 0, 0, 0);

  Node *toSpine =
      createNode("ToSpine", root, 3, -2.69724, 7.43032, -0.144315);
  Node *spine =
      createNode("Spine", toSpine, 3, -0.0310711, 10.7595, 1.96963);
  Node *spine1 = createNode("Spine1", spine, 3, 19.9056, 3.91189, 0.764692);

  Node *neck = createNode("Neck", spine1, 3, 25.9749, 7.03908, -0.130764);
  Node *head = createNode("Head", neck, 3, 9.52751, 0.295786, -0.907742);
  Node *top = createNode("Top", head, 3, 16.4037, 0.713936, 2.7358);

  /**/
  Node *leftShoulder =
      createNode("LeftShoulder", spine1, 3, 17.7449, 4.33886, 11.7777);
  Node *leftArm =
      createNode("LeftArm", leftShoulder, 3, 0.911315, 1.27913, 9.80584);
  Node *leftForeArm =
      createNode("LeftForeArm", leftArm, 3, 28.61265, 1.18197, -3.53199);
  Node *leftHand =
      createNode("LeftHand", leftForeArm, 3, 27.5088, 0.0218783, 0.327423);
  Node *endLeftHand =
      createNode("EndLHand", leftHand, 3, 18.6038, -0.000155887, 0.382096);

  /**/
  Node *rShoulder =
      createNode("RShoulder", spine1, 3, 17.1009, 2.89543, -12.2328);
  Node *rArm = createNode("RArm", rShoulder, 3, 1.4228, 0.178766, -10.211);
  Node *rForeArm = createNode("RForeArm", rArm, 3, 28.733, 1.87905, 2.64907);
  Node *rHand =
      createNode("RHand", rForeArm, 3, 27.4588, 0.290562, -0.101845);
  Node *endRHand =
      createNode("RLHand", rHand, 3, 17.8396, -0.255518, -0.000602873);

  Node *lUpLeg = createNode("LUpLeg", root, 3, -5.61296, -2.22332, -10.2353);
  Node *lLeg = createNode("LLeg", lUpLeg, 3, 2.56703, -44.7417, -7.93097);
  Node *lFoot = createNode("LFoot", lLeg, 3, 3.16933, -46.5642, -3.96578);
  Node *lToe = createNode("LToe", lFoot, 3, 0.346054, -6.02161, 12.8035);
  Node *lToe2 = createNode("LToe2", lToe, 3, 0.134235, -1.35082, 5.13018);

  Node *rUpLeg = createNode("RUpLeg", root, 3, -5.7928, -1.72406, 10.6446);
  Node *rLeg = createNode("RLeg", rUpLeg, 3, -2.57161, -44.7178, -7.85259);
  Node *rFoot = createNode("RFoot", rLeg, 3, -3.10148, -46.5936, -4.03391);
  Node *rToe = createNode("RToe", rFoot, 3, -0.0828122, -6.13587, 12.8035);
  Node *rToe2 = createNode("RToe2", rToe, 3, -0.131328, -1.35082, 5.13018);

  apply();
}

void freeTree() { freeNode(root); }

void freeNode(Node *node) {
  // printf("Freeing %s %p\n", node->name,node->children);
  if (node == NULL)
    return;
  // printf("Freeing children...\n");
  Node* ptr = node->children;
  Node* aux;
  while(ptr != NULL) {
    aux = ptr->next;
    freeNode(ptr);
    ptr = aux;
  }
  // printf("Freeing channel data...\n");
  free(node->channelData);
}

// **********************************************************************
//  Programa principal
// **********************************************************************
int main(int argc, char **argv) {

  FILE *file = fopen(argv[1], "r");
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH | GLUT_RGB);
  glutInitWindowPosition(0, 0);

  // Define o tamanho inicial da janela grafica do programa
  glutInitWindowSize(650, 500);

  // Cria a janela na tela, definindo o nome da
  // que aparecera na barra de título da janela.
  glutCreateWindow("BVH Viewer");

  // executa algumas inicializações
  init();

  // Exemplo: monta manualmente um esqueleto
  // (no trabalho, deve-se ler do arquivo)
  parseHierarchy(file);
  printHierarchy(root, 0);

    // Use o motionData conforme necessário
    // Exemplo: imprimir o primeiro frame
    for (int i = 0; i < frameDataSize; i++) {
        printf("Canal %d: %.2f\n", i, motionData[i]);
    }
  apply();


  // Define que o tratador de evento para
  // o redesenho da tela. A funcao "display"
  // será chamada automaticamente quando
  // for necessário redesenhar a janela
  glutDisplayFunc(display);

  // Define que a função que irá rodar a
  // continuamente. Esta função é usada para fazer animações
  // A funcao "display" será chamada automaticamente
  // sempre que for possível
  // glutIdleFunc ( display );

  // Define que o tratador de evento para
  // o redimensionamento da janela. A funcao "reshape"
  // será chamada automaticamente quando
  // o usuário alterar o tamanho da janela
  glutReshapeFunc(reshape);

  // Define que o tratador de evento para
  // as teclas. A funcao "keyboard"
  // será chamada automaticamente sempre
  // o usuário pressionar uma tecla comum
  glutKeyboardFunc(keyboard);

  // Define que o tratador de evento para
  // as teclas especiais(F1, F2,... ALT-A,
  // ALT-B, Teclas de Seta, ...).
  // A funcao "arrow_keys" será chamada
  // automaticamente sempre o usuário
  // pressionar uma tecla especial
  glutSpecialFunc(arrow_keys);

  // Registra a função callback para eventos de botões do mouse
  glutMouseFunc(mouse);

  // Registra a função callback para eventos de movimento do mouse
  glutMotionFunc(move);

  // inicia o tratamento dos eventos
  glutMainLoop();
}
