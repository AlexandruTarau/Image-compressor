/* TARAU Alexandru - 312CC */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    unsigned char r, g, b;
} Pixel; 

typedef struct Node {
    Pixel *pixel;
    struct Node *parent;
    struct Node *children[4];
    int depth;
} Node, *QuadTree;

typedef struct Item {
    QuadTree elem;
    struct Item *next;
} Item, *Queue;

QuadTree initQuadTree () { /* Initializes a QuadTree's root */
    QuadTree root;
    int i;
    root = (QuadTree)malloc(sizeof(Node));
    root->pixel = (Pixel*)malloc(sizeof(Pixel));
    root->parent = NULL;
    for (i = 0; i < 4; i++) {
        root->children[i] = NULL;
    }
    root->depth = 1;

    return root;
}

Pixel **initMatrix (int height, int width) { /* Initializes an image matrix */
    Pixel **a;
    int i;
    a = (Pixel**)malloc(height * sizeof(Pixel*));
    for (i = 0; i < height; i++) {
        a[i] = (Pixel*)malloc(width * sizeof(Pixel));
    }

    return a;
}

void readMatrix (Pixel **a, int height, int width, FILE *f) { /* Reads image */
    int i, j;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            fread(&a[i][j], sizeof(Pixel), 1, f);
        }
    }
}

QuadTree createChildren (Node *parent) { /* Creates children for a tree node */
    int i, j;
    
    for (i = 0; i < 4; i++) {
        Node *new = (QuadTree)malloc(sizeof(Node));
        new->pixel = (Pixel*)malloc(sizeof(Pixel));
        parent->children[i] = new;
        new->parent = parent;
        for (j = 0; j < 4; j++) {
            new->children[j] = NULL;
        }
    }

    return parent;
}

int maxDepth (QuadTree g) { /* Calculates max depth of tree */
    int i, max_child_depth = 0, child_depth;
    
    if (g == NULL) {
        return 0;
    }
    if (g->children[0] == NULL) {
        return g->depth;
    }
    for (i = 0; i < 4; i++) {
        child_depth = maxDepth(g->children[i]);
        if (child_depth > max_child_depth) {
            max_child_depth = child_depth;
        }
    }
    return max_child_depth;
    
}

int minDepth (QuadTree g) { /* Calculates min depth (closest leaf to root) */
    int depth, mindepth = 1000000, i;

    if (g == NULL) {
        return 0;
    }
    if (g->children[0] == NULL) {
        return g->depth;
    }
    for (i = 0; i < 4; i++) {
        depth = minDepth(g->children[i]);
        if (depth < mindepth) {
            mindepth = depth;
        }
    }
    return mindepth;
}

/* Creates a QuadTree for an image represented as a matrix of pixels */
QuadTree createQuadTree (QuadTree gr, int width, int height, Pixel **a,
                         int x, int y, int depth, int *nr, int factor) {
    int i, j;
    long long int red = 0, green = 0, blue = 0, mean = 0;
    unsigned char r, g, b;

    /* Using the formulas provided, we calculate the "score" of each block
    of the matrix */
    for (i = x; i < x + width; i++) {
        for (j = y; j < y + height; j++) {
            red += a[i][j].r;
            green += a[i][j].g;
            blue += a[i][j].b;
        }
    }
    r = (unsigned char)(red / (width * height));
    g = (unsigned char)(green / (width * height));
    b = (unsigned char)(blue / (width * height));

    for (i = x; i < x + width; i++) {
        for (j = y; j < y + height; j++) {
            mean += (r - a[i][j].r) * (r - a[i][j].r) +
                    (g - a[i][j].g) * (g - a[i][j].g) +
                    (b - a[i][j].b) * (b - a[i][j].b);
        }
    }
    mean = mean / (3 * width * height);

    if (mean <= factor) {
        /* "Fill" the node with color */
        gr->pixel->r = r;
        gr->pixel->g = g;
        gr->pixel->b = b;
        gr->depth = depth;
        (*nr)++;
        return gr;
    } else {
        /* "Divide" the node in 4 other nodes */
        gr = createChildren(gr);
        gr->children[0] = createQuadTree(gr->children[0], width / 2, height / 2,
                                         a, x, y, depth + 1, nr, factor);

        gr->children[1] = createQuadTree(gr->children[1], width / 2, height / 2,
                                         a, x, y + height / 2, depth + 1, nr,
                                         factor);

        gr->children[2] = createQuadTree(gr->children[2], width / 2, height / 2,
                                         a, x + width / 2, y + height / 2,
                                         depth + 1, nr, factor);

        gr->children[3] = createQuadTree(gr->children[3], width / 2, height / 2,
                                         a, x + width / 2, y, depth + 1, nr,
                                         factor);
        return gr;
    }

}

void freeQuadTree (QuadTree gr) { /* Frees the memory allocated for the tree */
    int i;
    if (gr) {
        for (i = 0; i < 4; i++) {
            freeQuadTree(gr->children[i]);
        }
        free(gr->pixel);
        free(gr);
    }
}

int power (int x, int p) { /* Calculates x to the power of p */
    int i, cp = x;
    if (p == 0) {
        return 1;
    }
    for (i = 1; i < p; i++) {
        x *= cp;
    }
    return x;
}

void addQ (QuadTree Node, Queue *front, Queue *rear) { /* Adds node to queue */
    Queue q = (Queue)malloc(sizeof(Item));
    q->elem = Node;
    q->next = NULL;
    if (!(*front) && !(*rear)) {
        (*front) = q;
        (*rear) = q;
        return;
    }
    (*rear)->next = q;
    (*rear) = q;
}

QuadTree delQ (Queue *front, Queue *rear) { /* Removes node from queue */
    Queue q = *front;
    QuadTree gr;

    if ((*front) == NULL) {
        return NULL;
    }
    gr = (*front)->elem;
    if ((*front) == (*rear)) {
        (*front) = NULL;
        (*rear) = NULL;
    } else {
        (*front) = (*front)->next;
    }
    free(q);
    return gr;
}

void compression (QuadTree gr, FILE *fw) { /* Writes the compression file */
    unsigned char tip;
    int i;
    Queue front = NULL, rear = NULL;
    QuadTree g;
    addQ(gr, &front, &rear);

    /* Using the BFS algorithm to write the data from each node of the tree */
    while (front) {
        g = delQ(&front, &rear);
        if (g->children[0] == NULL) {
            tip = 1;
            fwrite(&tip, sizeof(unsigned char), 1, fw);
            fwrite(&g->pixel->r, sizeof(unsigned char), 1, fw);
            fwrite(&g->pixel->g, sizeof(unsigned char), 1, fw);
            fwrite(&g->pixel->b, sizeof(unsigned char), 1, fw);
        } else {
            tip = 0;
            fwrite(&tip, sizeof(unsigned char), 1, fw);
            for (i = 0; i < 4; i++) {
                addQ(g->children[i], &front, &rear);
            }
        }
    }
}

/* Creates the matrix of the image stored in a quadtree */
Pixel **createMatrix (QuadTree gr, int width, int height,
                      Pixel **a, int x, int y) {
    int i, j;
    if (gr->children[0] == NULL) {
        for (i = x; i < width + x; i++) {
            for (j = y; j < height + y; j++) {
                a[i][j] = *(gr->pixel);
            }
        }
    } else {
        a = createMatrix (gr->children[0], width / 2, height / 2, a, x, y);
        a = createMatrix (gr->children[1], width / 2, height / 2, a, x, y + height / 2);
        a = createMatrix (gr->children[2], width / 2, height / 2, a, x + width / 2, y + height / 2);
        a = createMatrix (gr->children[3], width / 2, height / 2, a, x + width / 2, y);
    }
    return a;
}

int main (int argc, char **argv) {
    FILE *f;
    FILE *fw;
    char magic_number[3];
    int width, height, max_value, i, j, nr = 0, depth, BigSubmatrixSize, factor;
    Pixel **a;
    QuadTree root;
    
    /* Initialize the root of QuadTree */
    root = initQuadTree();

    if (!strcmp(argv[1], "-c1")) { /* Cerinta 1 */
        f = fopen(argv[3], "rb");
        fw = fopen(argv[4], "w");
        factor = atoi(argv[2]);

        /* Reading Header of .ppm file */
        fscanf(f, "%s\n%d %d\n%d", magic_number, &width, &height, &max_value);
        fseek(f, 1, SEEK_CUR);

        if (magic_number[0] != 'P' || magic_number[1] != '6') {
            printf("Only P6 format permited!");
            exit(1);
        }

        /* Initialize + read the matrix that make up the image */
        a = initMatrix(height, width);
        readMatrix(a, height, width, f);
        
        /* Create the tree based on the matrix and collect data from it */
        createQuadTree(root, width, height, a, 0, 0, root->depth, &nr, factor);
        depth = maxDepth(root);
        BigSubmatrixSize = height / (power(2, minDepth(root) - 1));
        fprintf(fw, "%d\n%d\n%d\n", depth, nr, BigSubmatrixSize);

    } else if (!strcmp(argv[1], "-c2")) { /* Cerinta 2 */
        f = fopen(argv[3], "rb");
        fw = fopen(argv[4], "wb");
        factor = atoi(argv[2]);

        /* Reading Header of .ppm file */
        fscanf(f, "%s\n%d %d\n%d", magic_number, &width, &height, &max_value);
        fseek(f, 1, SEEK_CUR);

        if (magic_number[0] != 'P' || magic_number[1] != '6') {
            printf("Only P6 format permited!");
            exit(1);
        }

        /* Initialize + read the matrix that make up the image */
        a = initMatrix(height, width);
        readMatrix(a, height, width, f);

        /* Create the tree based on the matrix and compress image */
        createQuadTree(root, width, height, a, 0, 0, root->depth, &nr, factor);
        fwrite(&height, sizeof(unsigned int), 1, fw);
        compression(root, fw);

    } else if (!strcmp(argv[1], "-d")) { /* Cerinta 3 */
        unsigned char tip;
        QuadTree g;
        f = fopen(argv[2], "rb");
        fw = fopen(argv[3], "wb");
        
        /* Create a queue for the tree nodes */
        Queue front = NULL, rear = NULL;
        addQ(root, &front, &rear);

        /* Read size of image */
        fread(&height, sizeof(unsigned int), 1, f);
        width = height;

        /* Create the tree using the BFS algorithm */
        while (front) {
            g = delQ(&front, &rear);

            fread(&tip, sizeof(unsigned char), 1, f);
            if (tip == 0) {
                
                g = createChildren(g);
                for (i = 0; i < 4; i++) {
                    addQ(g->children[i], &front, &rear);
                    g->children[i]->depth = g->depth + 1;
                }
            } else {
                fread(g->pixel, sizeof(Pixel), 1, f);
            }
        }

        /* Initialize + create the matrix that make up the image */
        a = initMatrix(height, width);
        a = createMatrix(root, width, height, a, 0, 0);

        /* Writting Header of .ppm file */
        strcpy(magic_number, "P6");
        magic_number[2] = '\0';
        max_value = 255;
        fprintf(fw, "%s\n%d %d\n%d\n", magic_number, width, height, max_value);

        /* Writting the image in the .ppm file */
        for (i = 0; i < height; i++) {
            for (j = 0; j < height; j++) {
                fwrite(&a[i][j], sizeof(Pixel), 1, fw);
            }
        }
    }

    /* Memory freeing */
    for (i = 0; i < height; i++) {
        free(a[i]);
    }
    free(a);
    freeQuadTree(root);

    fclose(fw);
    fclose(f);
    return 0;
}