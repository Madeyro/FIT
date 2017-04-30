	
/* c206.c **********************************************************}
{* Téma: Dvousmìrnì vázaný lineární seznam
**
**                   Návrh a referenèní implementace: Bohuslav Køena, øíjen 2001
**                            Pøepracované do jazyka C: Martin Tuèek, øíjen 2004
**                                            Úpravy: Bohuslav Køena, øíjen 2015
**
** Implementujte abstraktní datový typ dvousmìrnì vázaný lineární seznam.
** U¾iteèným obsahem prvku seznamu je hodnota typu int.
** Seznam bude jako datová abstrakce reprezentován promìnnou
** typu tDLList (DL znamená Double-Linked a slou¾í pro odli¹ení
** jmen konstant, typù a funkcí od jmen u jednosmìrnì vázaného lineárního
** seznamu). Definici konstant a typù naleznete v hlavièkovém souboru c206.h.
**
** Va¹ím úkolem je implementovat následující operace, které spolu
** s vý¹e uvedenou datovou èástí abstrakce tvoøí abstraktní datový typ
** obousmìrnì vázaný lineární seznam:
**
**      DLInitList ...... inicializace seznamu pøed prvním pou¾itím,
**      DLDisposeList ... zru¹ení v¹ech prvkù seznamu,
**      DLInsertFirst ... vlo¾ení prvku na zaèátek seznamu,
**      DLInsertLast .... vlo¾ení prvku na konec seznamu, 
**      DLFirst ......... nastavení aktivity na první prvek,
**      DLLast .......... nastavení aktivity na poslední prvek, 
**      DLCopyFirst ..... vrací hodnotu prvního prvku,
**      DLCopyLast ...... vrací hodnotu posledního prvku, 
**      DLDeleteFirst ... zru¹í první prvek seznamu,
**      DLDeleteLast .... zru¹í poslední prvek seznamu, 
**      DLPostDelete .... ru¹í prvek za aktivním prvkem,
**      DLPreDelete ..... ru¹í prvek pøed aktivním prvkem, 
**      DLPostInsert .... vlo¾í nový prvek za aktivní prvek seznamu,
**      DLPreInsert ..... vlo¾í nový prvek pøed aktivní prvek seznamu,
**      DLCopy .......... vrací hodnotu aktivního prvku,
**      DLActualize ..... pøepí¹e obsah aktivního prvku novou hodnotou,
**      DLSucc .......... posune aktivitu na dal¹í prvek seznamu,
**      DLPred .......... posune aktivitu na pøedchozí prvek seznamu, 
**      DLActive ........ zji¹»uje aktivitu seznamu.
**
** Pøi implementaci jednotlivých funkcí nevolejte ¾ádnou z funkcí
** implementovaných v rámci tohoto pøíkladu, není-li u funkce
** explicitnì uvedeno nìco jiného.
**
** Nemusíte o¹etøovat situaci, kdy místo legálního ukazatele na seznam 
** pøedá nìkdo jako parametr hodnotu NULL.
**
** Svou implementaci vhodnì komentujte!
**
** Terminologická poznámka: Jazyk C nepou¾ívá pojem procedura.
** Proto zde pou¾íváme pojem funkce i pro operace, které by byly
** v algoritmickém jazyce Pascalovského typu implemenovány jako
** procedury (v jazyce C procedurám odpovídají funkce vracející typ void).
**/

#include "c206.h"

int solved;
int errflg;

void DLError() {
/*
** Vytiskne upozornìní na to, ¾e do¹lo k chybì.
** Tato funkce bude volána z nìkterých dále implementovaných operací.
**/	
    printf ("*ERROR* The program has performed an illegal operation.\n");
    errflg = TRUE;             /* globální promìnná -- pøíznak o¹etøení chyby */
    return;
}

void DLInitList (tDLList *L) {
/*
** Provede inicializaci seznamu L pøed jeho prvním pou¾itím (tzn. ¾ádná
** z následujících funkcí nebude volána nad neinicializovaným seznamem).
** Tato inicializace se nikdy nebude provádìt nad ji¾ inicializovaným
** seznamem, a proto tuto mo¾nost neo¹etøujte. V¾dy pøedpokládejte,
** ¾e neinicializované promìnné mají nedefinovanou hodnotu.
**/
    
	L->Act = NULL;
    L->First = NULL;
    L->Last = NULL;
}

void DLDisposeList (tDLList *L) {
/*
** Zru¹í v¹echny prvky seznamu L a uvede seznam do stavu, v jakém
** se nacházel po inicializaci. Ru¹ené prvky seznamu budou korektnì
** uvolnìny voláním operace free. 
**/
	
    tDLElemPtr temp = L->First;   /* Prave uvolnovany prvok */
    tDLElemPtr ntemp = temp;      /* Prvok, kt. nasleduje za uvolnovanym prvkom */
    
    while (temp != NULL) {
        ntemp = temp->rptr;
        free(temp);
        temp = ntemp;
    }
    L->First = NULL;
    L->Last = NULL;
    L->Act = NULL;
}

void DLInsertFirst (tDLList *L, int val) {
/*
** Vlo¾í nový prvek na zaèátek seznamu L.
** V pøípadì, ¾e není dostatek pamìti pro nový prvek pøi operaci malloc,
** volá funkci DLError().
**/
	
    tDLElemPtr temp;
    
    if ((temp = malloc(sizeof(struct tDLElem))) == NULL)  DLError();
    else {
        temp->data = val;
        if (L->First == NULL) {
            temp->rptr = NULL;
            temp->lptr = NULL;
            L->Last = temp;
            /* ^^^
             * Ak je zoznam na zaciatku prazdny je potrebne nastavit 
             * nasledujuci a predchadzajuci prvok na NULL, 
             * pretoze pri inicializacii to nie je mozne.
             * Ak bol predtym prazdny zoznam tak prvy prvok je zaroven posledny. 
             */
        }
        else {
            temp->rptr = L->First;
            temp->lptr = NULL;
            L->First->lptr = temp;
        }
        L->First = temp;
    }
}

void DLInsertLast(tDLList *L, int val) {
/*
** Vlo¾í nový prvek na konec seznamu L (symetrická operace k DLInsertFirst).
** V pøípadì, ¾e není dostatek pamìti pro nový prvek pøi operaci malloc,
** volá funkci DLError().
**/ 	
	
	tDLElemPtr temp;
    
    if ((temp = malloc(sizeof(struct tDLElem))) == NULL)  DLError();
    else {
        temp->data = val;
        if (L->First == NULL) {
            temp->rptr = NULL;
            temp->lptr = NULL;
            L->First = temp;
            /* ^^^
             * Ak je zoznam na zaciatku prazdny je potrebne nastavit 
             * nasledujuci a predchadzajuci prvok na NULL, 
             * pretoze pri inicializacii to nie je mozne.
             * Ak bol predtym prazdny zoznam tak prvy prvok je zaroven posledny.
             */
        }
        else {
            temp->rptr = NULL;
            temp->lptr = L->Last;
            L->Last->rptr = temp;
        }
        L->Last = temp;
    }
}

void DLFirst (tDLList *L) {
/*
** Nastaví aktivitu na první prvek seznamu L.
** Funkci implementujte jako jediný pøíkaz (nepoèítáme-li return),
** ani¾ byste testovali, zda je seznam L prázdný.
**/
	
    L->Act = L->First;
}

void DLLast (tDLList *L) {
/*
** Nastaví aktivitu na poslední prvek seznamu L.
** Funkci implementujte jako jediný pøíkaz (nepoèítáme-li return),
** ani¾ byste testovali, zda je seznam L prázdný.
**/
	
	L->Act = L->Last;
}

void DLCopyFirst (tDLList *L, int *val) {
/*
** Prostøednictvím parametru val vrátí hodnotu prvního prvku seznamu L.
** Pokud je seznam L prázdný, volá funkci DLError().
**/
	
	if (L->First == NULL)   DLError();
    else                    *val = L->First->data;
}

void DLCopyLast (tDLList *L, int *val) {
/*
** Prostøednictvím parametru val vrátí hodnotu posledního prvku seznamu L.
** Pokud je seznam L prázdný, volá funkci DLError().
**/
	
	if (L->First == NULL)    DLError();
    else                     *val = L->Last->data;
}

void DLDeleteFirst (tDLList *L) {
/*
** Zru¹í první prvek seznamu L. Pokud byl první prvek aktivní, aktivita 
** se ztrácí. Pokud byl seznam L prázdný, nic se nedìje.
**/
	
	tDLElemPtr temp;
    
    if (L->First != NULL) {
        if (L->Act == L->First)     L->Act = NULL; /* zrusenie aktivnosti */
        if (L->First != L->Last) {   /* Ak ma zoznam viac ako jeden prvok */
            temp = L->First->rptr;   /* temp ukazuje na prvok za prvym prvkom */
            free(L->First);
            temp->lptr = NULL;
            L->First = temp;
        }
        else {                       /* Ak ma zoznam prave jeden prvok */
            free(L->First);
            L->First = NULL;
            L->Last = NULL;
        }
    }
}	

void DLDeleteLast (tDLList *L) {
/*
** Zru¹í poslední prvek seznamu L. Pokud byl poslední prvek aktivní,
** aktivita seznamu se ztrácí. Pokud byl seznam L prázdný, nic se nedìje.
**/ 
	
    tDLElemPtr temp;
    
    if (L->First != NULL) {
        if (L->Act == L->Last)     L->Act = NULL; /* zrusenie aktivnosti */
        if (L->First != L->Last) {  /* Ak ma zoznam viac ako jeden prvok */
            temp = L->Last->lptr;   /* temp ukazuje na predposledny prvok */
            free(L->Last);
            temp->rptr = NULL;
            L->Last = temp;
        }
        else {                      /* Ak ma zoznam prave jeden prvok */
            free(L->Last);
            L->First = NULL;
            L->Last = NULL;
        }
    }
}

void DLPostDelete (tDLList *L) {
/*
** Zru¹í prvek seznamu L za aktivním prvkem.
** Pokud je seznam L neaktivní nebo pokud je aktivní prvek
** posledním prvkem seznamu, nic se nedìje.
**/
	
    tDLElemPtr temp;
    
    if (L->Act != NULL)
    if (L->Act != L->Last) {
        if (L->Act->rptr->rptr == NULL)
            L->Last = L->Act;   /* Ak je po odstraneny aktivny prvok posledny */
        temp = L->Act->rptr->rptr;    /* Ukazuje na druhy prvok za aktivnym */
        free(L->Act->rptr);           /* Uvolni prvok za aktivnym */
        L->Act->rptr = temp;
        if (L->Act->rptr != NULL)   /* Ak nebol prvok pred aktivnym prvy */
            L->Act->rptr->lptr = L->Act;  /* Zabezpecenie plynulosti */
        /* ^^^
         * L->Act->rptr uz ukazuje na prvok za zmazanym, musime vsak
         * nastavit aby lavy ukazovatel prvku za zmazanym prvkom ukazoval
         * na aktivny prvok.
         */
    }
}

void DLPreDelete (tDLList *L) {
/*
** Zru¹í prvek pøed aktivním prvkem seznamu L .
** Pokud je seznam L neaktivní nebo pokud je aktivní prvek
** prvním prvkem seznamu, nic se nedìje.
**/
    
    tDLElemPtr temp;
    
    if (L->Act != NULL)
    if (L->Act != L->First) {
        if (L->Act->lptr->lptr == NULL) 
            L->First = L->Act;  /* Ak je po odstraneni aktivny prvok prvy */
        temp = L->Act->lptr->lptr;    /* Ukazuje na druhy prvok pred aktivnym */
        free(L->Act->lptr);           /* Uvolni prvok pred aktivnym */
        L->Act->lptr = temp;
        if (L->Act->lptr != NULL)   /* Ak nebol prvok pred aktivnym prvy */
            L->Act->lptr->rptr = L->Act;  /* Zabezpecenie plynulosti */
        /* ^^^
         * L->Act->lptr uz ukazuje na prvok pred zmazanym, musime vsak
         * nastavit aby pravy ukazovatel prvku pred zmazanym prvkom ukazoval
         * na aktivny prvok.
         */
    }
}

void DLPostInsert (tDLList *L, int val) {
/*
** Vlo¾í prvek za aktivní prvek seznamu L.
** Pokud nebyl seznam L aktivní, nic se nedìje.
** V pøípadì, ¾e není dostatek pamìti pro nový prvek pøi operaci malloc,
** volá funkci DLError().
**/
	
	tDLElemPtr temp;
    
    if (L->Act != NULL) {
        if ((temp = malloc(sizeof(struct tDLElem))) == NULL)      DLError();
        temp->data = val;
        temp->lptr = L->Act; 
        /* ^^^ 
         * Predchadzajuci prvok noveho je aktivny prvok 
         */
        temp->rptr = L->Act->rptr;
        /* ^^^ 
         * Nasledujuci prvok noveho je prvok nasledujuci za aktivnym prvkom 
         */
        if (L->Act->rptr == NULL)
            L->Last = temp; /* Ak je aktivny prvok zaroven poslednym prvkom */
        else
            L->Act->rptr->lptr = temp;
        /* ^^^ 
         * Prvok za aktivnym je teraz za pridanym prvkom
         */
        L->Act->rptr = temp;
        /* ^^^
         * Nasledujuci prvok za aktivnym je prave pridany prvok
         */
    }
}

void DLPreInsert (tDLList *L, int val) {
/*
** Vlo¾í prvek pøed aktivní prvek seznamu L.
** Pokud nebyl seznam L aktivní, nic se nedìje.
** V pøípadì, ¾e není dostatek pamìti pro nový prvek pøi operaci malloc,
** volá funkci DLError().
**/
	
	tDLElemPtr temp;
    
    if (L->Act != NULL) {
        if ((temp = malloc(sizeof(struct tDLElem))) == NULL)      DLError();
        temp->data = val;
        temp->rptr = L->Act; 
        /* ^^^ 
         * Nasledujuci prvok noveho je aktivny prvok 
         */
        temp->lptr = L->Act->lptr;
        /* ^^^ 
         * Predchadzajuci prvok noveho je prvok pred aktivnym prvkom 
         */
        if (L->Act->lptr == NULL)
            L->First = temp; /* Ak je aktivny prvok zaroven prvym prvkom */
        else
            L->Act->lptr->rptr = temp;
        /* ^^^ 
         * Prvok pred aktivnym je teraz pred pridanym
         */
        L->Act->lptr = temp;
        /* ^^^
         * Prave pridany sa teraz nachadza pred aktivnym
         */
    }
}

void DLCopy (tDLList *L, int *val) {
/*
** Prostøednictvím parametru val vrátí hodnotu aktivního prvku seznamu L.
** Pokud seznam L není aktivní, volá funkci DLError ().
**/
		
	if (L->Act != NULL)     *val = L->Act->data;
    else                    DLError();
}

void DLActualize (tDLList *L, int val) {
/*
** Pøepí¹e obsah aktivního prvku seznamu L.
** Pokud seznam L není aktivní, nedìlá nic.
**/
	
    if (L->Act != NULL) {
        L->Act->data = val;
    }
}

void DLSucc (tDLList *L) {
/*
** Posune aktivitu na následující prvek seznamu L.
** Není-li seznam aktivní, nedìlá nic.
** V¹imnìte si, ¾e pøi aktivitì na posledním prvku se seznam stane neaktivním.
**/
	
    if (L->Act != NULL) {
        L->Act = L->Act->rptr;
    }
}


void DLPred (tDLList *L) {
/*
** Posune aktivitu na pøedchozí prvek seznamu L.
** Není-li seznam aktivní, nedìlá nic.
** V¹imnìte si, ¾e pøi aktivitì na prvním prvku se seznam stane neaktivním.
**/
	
    if (L->Act != NULL) {
        L->Act = L->Act->lptr;
    }
}

int DLActive (tDLList *L) {
/*
** Je-li seznam L aktivní, vrací nenulovou hodnotu, jinak vrací 0.
** Funkci je vhodné implementovat jedním pøíkazem return.
**/
	
	return L->Act != NULL ? 1 : 0;
}

/* Konec c206.c*/
