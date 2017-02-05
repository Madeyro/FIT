/*
================================================================================
Projekt 3: Pruchod bludistem
Autor: Maros Kopec
Login: xkopec44
Osobni cislo VUT: 175295
Datum: december 2014
================================================================================
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

enum {TEST, RPATH, LPATH, SHORTEST, NON};
enum {FTEST=2, FPATH=4};
enum {LEFT=1, RIGHT=2, HORIZONTAL=4};
enum {ERR_ARG=2, ERR_FILE, ERR_MALLOC, ERR_START_BORDED, ERR_OPERATION, 
      ERR_RPATH, ERR_LPATH, ERR_INVALID};

typedef struct args
{
    int operation;
    int row;
    int col;
    int fplace;
} TArgs;

typedef struct map
{
    int rows;
    int cols;
    unsigned char *cells;
} Map;

/*  
 *  Functions are commented each individual, right above their definitions
 */
bool isodd(int num);
bool isupwards(int r, int c);
bool isborder(Map *map, int r, int c, int border);
int start_border(Map *map, int r, int c, int leftright);
bool test(Map *map, int *errcode);
int fill_map(FILE *f, Map *map);
void free_map(Map *map);
int find_path(Map *map, int border, int r, int c, int leftright);
int execute_finding(int operation, Map *map, int r, int c);
int grab_right(bool triodd, int direction);
int grab_left(bool triodd, int direction);
void print_help();
void print_error(int errcode);
int ch_arg(int argc, char const *argv[], TArgs *arr);
int do_operation(int operation, Map *map, int r, int c, int *errcode);

/*
================================================================================
~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~ M A I N ~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
================================================================================
*/
int main(int argc, char const *argv[])
{
    TArgs arr;
    Map map;
    FILE *f;
    int errcode = 0;

    if (strcmp(argv[1],"--help") == 0 && argc == 2)
    {
        print_help();
        return EXIT_SUCCESS;
    }
    else if (ch_arg(argc, argv, &arr) == ERR_ARG)
    {
        print_error(ERR_ARG);
        return EXIT_FAILURE;
    }
    else
        if ( (f = fopen(argv[arr.fplace],"r")) == NULL)
        {
            print_error(ERR_FILE);
            return EXIT_FAILURE;
        }

    if ( (errcode = fill_map(f, &map)) == ERR_MALLOC )
    {
        print_error(ERR_MALLOC);
        fclose(f);
        return EXIT_FAILURE;
    }

    if (do_operation(arr.operation, &map, arr.row, arr.col, &errcode) 
        == ERR_OPERATION)
    {
        print_error(ERR_OPERATION);
        free_map(&map);
        fclose(f);
        return EXIT_FAILURE;
    }

    free_map(&map);
    fclose(f);
    return EXIT_SUCCESS;
}
/*
================================================================================
~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*~*
================================================================================
*/

/*  
 *  Function: do_operation
 *
 *  Execute operations according to arguments
 */
int do_operation(int operation, Map *map, int r, int c, int *errcode)
{
    if (operation == TEST)
    {
        if ( test(map, errcode) )
            printf("Valid\n");
        else
            printf("Invalid\n");
    }
    else if (operation == SHORTEST)
    {
        printf("Turn around and use entry as exit...\n");
    }
    else if (operation == RPATH || operation == LPATH)
    {
        if ( !(test(map, errcode)) ) //test return true if map is valid
        {
            print_error(ERR_INVALID);
            return ERR_INVALID;
        }    
        if ( execute_finding(operation, map, r, c) == ERR_OPERATION )
        {
            return ERR_OPERATION;
        }
    }
    return EXIT_SUCCESS;
}

/*  
 *  Function: execute_finding
 *
 *  Sets parameters to other functions 
 *  according to right hand and left hand rule
 */
int execute_finding(int operation, Map *map, int r, int c)
{
    int tmperr;
    int border;

    if ( (border = start_border(map, r, c, operation)) == ERR_START_BORDED )
    {
        print_error(ERR_START_BORDED);
        return ERR_OPERATION;
    }
    tmperr = find_path(map, border, r, c, operation);
    if (tmperr == LPATH)
    {
        print_error(LPATH);
        return ERR_OPERATION;
    }
    if (tmperr == RPATH)
    {
        print_error(RPATH);
        return ERR_OPERATION;
    }
    return EXIT_SUCCESS;
}

/*  
 *  Function: find_path
 *
 *  Will find path throught matrix by left or right hand rule
 *  The rule is specified by leftright parameter
 */
int find_path(Map *map, int border, int r, int c, int leftright)
{
    bool trapped = true;

    do
    {
        if ( isborder(map, r, c, border) )
            //tested border is uncrossable, we are rotating in triangle
            if ( (leftright == RPATH && (!(isupwards(r, c)))) 
                || (leftright == LPATH && (isupwards(r, c))) )   
            {
                if (border == HORIZONTAL)
                    border = border >> 2;
                else
                    border = border << 1;
            }
            else
            {
                if (border == LEFT)
                    border = border << 2;
                else
                    border = border >> 1;   
            }
        else
        {
            //tested border is crossable
            printf("%d,%d\n", r+1, c+1);
            if (border == LEFT)
                c--;
            if (border == RIGHT)
                c++;
            if (border == HORIZONTAL)
            { 
                if (isupwards(r, c))
                    r++;
                else
                    r--;
            }
            if (r < 0 || r == map->rows || c < 0 || c == map->cols)
                trapped = false;
            if (leftright == RPATH)
                border = grab_right(isupwards(r, c), border);
            else
                border = grab_left(isupwards(r, c), border);
            if ( border == ERR_RPATH )
                return ERR_RPATH;
            if ( border == ERR_LPATH )
                return ERR_LPATH;
        }
    } while (trapped);

    return EXIT_SUCCESS;
}

/*  
 *  Function: grab_right
 *
 *  Sets, which border will right hand grab after entering triangle
 *  Parameter upwards specifies, if triangel is pointing up or down
 */
int grab_right(bool upwards, int direction)
{
    if (direction == RIGHT)
        return upwards ? HORIZONTAL : RIGHT;
    if (direction == LEFT)
        return upwards ? LEFT : HORIZONTAL;
    if (direction == HORIZONTAL)
        return upwards ? RIGHT : LEFT;
    return ERR_RPATH;
}

/*  
 *  Function: grab_left
 *
 *  Sets, which border will left hand grab after entering triangle
 *  Parameter upwards specifies, if triangel is pointing up or down
 */
int grab_left(bool upwards, int direction)
{
    if (direction == RIGHT)
        return upwards ? RIGHT : HORIZONTAL;
    if (direction == LEFT)
        return upwards ? HORIZONTAL : LEFT;
    if (direction == HORIZONTAL)
        return upwards ? LEFT : RIGHT;
    return ERR_LPATH;
}

/*  
 *  Function: start_border
 *
 *  Sets, which border will hand grab after entering maze
 *  Parameter leftright specifies, which hand are going to use
 */
int start_border(Map *map, int r, int c, int leftright)
{
    //from left
    //in function isodd() is row increased by one
    //because user sets rows with starting index 1
    //but program calculate with rows index starting with 0
    if ( (c == 0) && !(isodd(r+1)) )
        if ( !(isborder(map, r, c, LEFT)) )        
            //when RPATH is called returns HORIZONTAL
            //when LPATH is called returns RIGHT
            //same applies for conditions below
            return (leftright == RPATH) ? HORIZONTAL : RIGHT;
    //from left
    if ( (c == 0) && isodd(r+1) )
        if ( !(isborder(map, r, c, LEFT)) )   
            return (leftright == RPATH) ? RIGHT : HORIZONTAL;
    //from right
    if ( (c == map->cols - 1 ) && (isupwards(r, c)) )
        if ( !(isborder(map, r, c, RIGHT)) )
            return (leftright == RPATH) ? LEFT : HORIZONTAL;
    //from right
    if ( (c == map->cols - 1 ) && !(isupwards(r, c)) )
        if ( !(isborder(map, r, c, RIGHT)) )
            return (leftright == RPATH) ? HORIZONTAL : LEFT;
    //from bottom
    if ( r == map->rows - 1 )
        if ( !(isborder(map, r, c, HORIZONTAL)) )
            return (leftright == RPATH) ? RIGHT : LEFT;
    //from above
    if ( r == 0 )
        if ( !(isborder(map, r, c, HORIZONTAL)) )
            return (leftright == RPATH) ? LEFT : RIGHT;
    return ERR_START_BORDED;
}

/*  
 *  Function: test
 *
 *  Returns true if map (maze) is valid
 *  Returns false if map (maze) is invalid (not valid)
 */
bool test(Map *map, int *errcode)
{
    if (*errcode == ERR_INVALID)
        //errcode is set to ERR_INVALID, if an error occuered
        //in function fill_map()
        return false;
    for (int r = 0; r < map->rows; ++r)
    {
        for (int c = 0; c < map->cols; ++c)
        {
            if (c != map->cols-1)
            {
                if ( (isborder(map, r, c, RIGHT) 
                     ^ isborder(map, r, c+1, LEFT)) )
                {
                    return false;
                }
            }
            if ( (isupwards(r, c)) && (r != map->rows-1) )
            {
                if ( (isborder(map, r, c, HORIZONTAL)) 
                     ^ (isborder(map, r+1, c, HORIZONTAL)) )
                {
                    return false;
                }
            }
        }
    }
    return true;
}

/*  
 *  Function: isborder
 *
 *  Returns true if in triangle is specified border uncrossable
 *  Parameter border specifies, which wall are we testing
 */
bool isborder(Map *map, int r, int c, int border)
{
    return (map->cells[r * map->cols + c] & border) > 0 ? true : false;
}

/*  
 *  Function: isodd
 *
 *  Returns true if number is odd
 *  (last bit is equal to 1)
 */
bool isodd(int num)
{
    return ((num & 001) > 0) ? true : false;
}

/*  
 *  Function: isupwards
 *
 *  Returns true if triangle is pointing upwards
 */
bool isupwards(int r, int c)
{
    return (isodd(r+c) > 0) ? true : false;
}

/*  
 *  Function: ch_arg
 *
 *  Returns true EXIT_SUCCES, if parsed arguments are OK
 *  Returns error ERR_ARG, if arguments are somehow parsed badly
 */
int ch_arg(int argc, char const *argv[], TArgs *arr)
{
    int row;
    int col;
    arr->operation = NON;

    if (strcmp(argv[1],"--test") == 0 && argc == 3)
    {
        arr->operation = TEST;
        arr->fplace = FTEST;
        return EXIT_SUCCESS;
    }
    else if ( argc == 5 )
    {
        if (strcmp(argv[1],"--rpath") == 0)
            arr->operation = RPATH;
        else if (strcmp(argv[1],"--lpath") == 0)
            arr->operation = LPATH;
        else if (strcmp(argv[1],"--shortest") == 0)
            arr->operation = SHORTEST;
        if (arr->operation == NON)
            return ERR_ARG;
        if ((row = atoi(argv[2])) > 0 && (col = atoi(argv[3])) > 0)
        {
            arr->row = row-1;
            arr->col = col-1;
            arr->fplace = FPATH;
            return EXIT_SUCCESS;
        }
        else
            return ERR_ARG;
    }
    else
        return ERR_ARG;
}

/*  
 *  Function: fill_map
 *
 *  Fills metrix that is type Map from opened file
 *  If map (maze) in file is invalid, returns ERR_INVALID
 */
int fill_map(FILE *f, Map *map)
{
    unsigned char tmp = 0;

    fscanf(f,"%d %d",&map->rows, &map->cols);
    map->cells = (unsigned char *)
                    malloc(map->rows * map->cols*sizeof(unsigned char));
    if (map->cells == NULL)
        return ERR_MALLOC;
    for (int i = 0; i < (map->rows * map->cols); ++i)
    {
        if ((fscanf(f,"%hhu", &map->cells[i])) != 1 
            || map->cells[i] > 7 )
        {
            return ERR_INVALID;
        }
    }
    if (fscanf(f,"%hhu",&tmp) != EOF)
        return ERR_INVALID;
    return EXIT_SUCCESS;
}

/*  
 *  Function: free_map
 *
 *  Frees the memory allocated for array cells in struct Map
 */
void free_map(Map *map)
{
    free(map->cells);
}

/*  
 *  Function: print_error
 *
 *  Writes error message on stderr and stdout
 */
void print_error(int errcode)
{
    char *error[] = {"EXIT_SUCCESS",
                    "EXIT_FAILURE",
                    "Wrong arguments!",
                    "Failed to open file. File does not exist or is corrupted",
                    "Failed to alloc memory",
                    "Failed to set starting border",
                    "Failed to execute operation",
                    "Failed at finding path by right hand rule",
                    "Failed at finding path by left hand rule",
                    "Error occuerd. Invalid map"
                    };

    fprintf(stderr, "%s\n", error[errcode]);
    return;
}

/*  
 *  Function: print_help
 *
 *  Writes short manual, for using this program, on stdout
 */
void print_help()
{
    /*
     *char *help ={
     *    "\nHelp, I need somebody.\n"
     *    "Help, not just anybody.\n"
     *    "Help, you know I need someone.\n"
     *    "Help!\n"
     *};
     */
    
    char *manual = {
        "\nDESCRIPTION\n\n"
        "\tThis is simple program find path through triangle metrix (maze).\n"
        "\tThe right or left hand rule is applied.\n"
        "\nSYNOPSIS\n\n"
        "\t./proj3 --help\tPrint this help on stdout\n"
        "\t./proj3 --test map.txt\tPrint Valid on stdout, "
        "if map is without errors\n"
        "\t./proj3 --rpath R C map\tPrint route throught maze by RIGHT"
        "hand rule\n\t\tR and C specify row and column of starting triangle\n"
        "\t./proj3 --rpath R C map\tPrint route throught maze by LEFT"
        "hand rule\n\t\tR and C specify row and column of starting triangle\n"
    };

    printf("%s\n", manual);
    return;
}