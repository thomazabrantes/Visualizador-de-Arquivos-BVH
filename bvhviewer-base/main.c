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

// Total de frames
int totalFrames;

// Frame atual
int curFrame = 0;

// Funcoes para liberacao de memoria da hierarquia
void freeTree();
void freeNode(Node *node);

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

//
// DADOS DE EXEMPLO DO PRIMEIRO FRAME
//

float data[] = {
    -326.552, 98.7701,   317.634,  71.4085,   60.8487,  17.2406,  -70.1915,
    0,        88.8779,   84.6529,  68.0632,   -5.27801, 0.719492, 15.2067,
    13.3733,  -135.039,  24.774,   172.053,   -171.896, 64.9682,  -165.105,
    3.6548,   1.03593,   -36.4128, -55.7886,  37.8019,  -120.338, 9.39682,
    14.0503,  -27.1815,  4.41274,  -0.125185, -1.52942, 1.33299,  -4.20935,
    46.1022,  -92.5385,  -35.676,  63.2656,   -5.23096, -15.2195, 9.30354,
    11.1114,  -0.982512, -11.0421, -86.4319,  -3.01435, 76.3394,  1.71268,
    24.9011,  -2.42099,  9.483,    17.5267,   -1.42749, -37.0021, -44.3019,
    -39.1702, -46.2538,  -2.58689, 78.4703,   1.9216,   29.8211,  -1.99744,
    -3.70506, 1.06523,   0.577189, 0.146783,  3.70013,  2.9702};

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
  applyData(data, root);
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
  initMaleSkel();

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
