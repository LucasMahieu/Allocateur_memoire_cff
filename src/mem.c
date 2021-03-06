/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"

typedef struct zone_mem 
{
    unsigned long taille_mem;
    struct zone_mem* suiv;
} *Liste;

// Variable globale qui représente la tête de liste
static Liste LZL = 0;    
static unsigned long mem_lib = 0;
void *zone_memoire = 0;
#define TAILLE_STRUCT sizeof((*LZL))

int mem_init()
{
//Si on init alors que le destroy n'a pas ete faites
    if (!zone_memoire)
    {
        mem_destroy();
    }

    Liste Liste_init = 0;

    // On alloue tout le bloc de mémoire.
    zone_memoire = (void *) malloc(ALLOC_MEM_SIZE);

    if (zone_memoire == 0) // Si l'allocation a échoué
    {
        //perror("mem_init:");
        return -1;
    }

    // On place la tête de liste au début du bloc alloué
    Liste_init = zone_memoire;
    // La taille du bloc mémoire comprend la taille de la cellule.
    Liste_init->taille_mem = ALLOC_MEM_SIZE;
    //Variable globale qui contient le nombre totale de mem libre
    mem_lib = ALLOC_MEM_SIZE;
    Liste_init->suiv=Liste_init;
    // Liste_init étant une variable locale, elle est supprimée à la fin
    // de la fonction.
    LZL = Liste_init;
    return 0;
}

void * mem_alloc(unsigned long size)
{
    Liste temp1 = LZL;
    Liste temp2;
    // Pas de zone libre
    if (LZL == 0 || mem_lib == 0)
    {
        LZL = 0;
        return (void *)0;
    }
    
    //Alloc de tout
    if ( (size == ALLOC_MEM_SIZE) && (mem_lib == ALLOC_MEM_SIZE) ) {
        LZL = 0;
        mem_lib = 0;
        return zone_memoire;
    }

    if (size == 0)
    {
        // On retourne une erreur en cas d'allocation de taille nulle.
        return (void *)0;
    }
    // On rend size un multiple de sizeof(*Liste) par facilité
    if (size % sizeof(*temp1))
    {
        size += (sizeof(*temp1) - (size % sizeof(*temp1)));
    }
    // On recherche une ZL de taille supérieure à la demande.
    while ((temp1->taille_mem) < size)
    {
        temp2 = temp1;
        temp1 = temp1->suiv;
        // On a en trouvé aucune.
        if (temp1 == LZL)
        {
            return (void *)0;
        }
    }

    // Cas où le premier bloc libre est pointé par LZL et occupe tout le bloc.
    if (temp1->taille_mem == size && temp1 == LZL)
    {
        temp2 = LZL;
        while(temp2->suiv != LZL)
        {
            temp2 = temp2->suiv;
        }
        LZL = LZL -> suiv;
        temp2->suiv = LZL;
        mem_lib -= size;
        return (void *)temp1;
    }

    // Cas où tout le bloc choisi doit être alloué : on doit supprimer une cell.
    if (temp1->taille_mem == size)
    {
        // Les éléments de temp1 sont toujours dans la mémoire mais plus suivis.
        temp2->suiv = temp1->suiv;
        mem_lib -= size;
        return (void *)temp1;
    }
    // Les autres cas : on alloue au "fond" du bloc dispo et on modifie la cell.
    temp1->taille_mem -= size;
    mem_lib -= size;
    return ((void *)temp1) + temp1->taille_mem;
}

int mem_free(void *ptr, unsigned long size)
{
    Liste z=ptr;
    Liste l=LZL;
    Liste q = ptr;

// On rend size un multiple de sizeof(*Liste) par facilité
    if (size % TAILLE_STRUCT) {
        size += (TAILLE_STRUCT - (size % TAILLE_STRUCT));
    }

    if(size == 0){
        //perror("mem_free:");
        return -1;
    }
// Demande de free en dehors de la zone memoire
    if( ptr < zone_memoire || ptr > (zone_memoire + ALLOC_MEM_SIZE)) {
        //perror("mem_free:");
        return -1;
    }
//Il n'y avait plus de LZL puisqu'il n'y avait plus de zone libre 
    if (mem_lib == 0) {
        q->suiv = q;
        q->taille_mem = size;
        LZL = q;
        mem_lib = size;
        return 0;
    }

// On va placer l sur la ZL juste avant la zone à liberer
    for(l=LZL;l->suiv < z;l=l->suiv){
        //Si la liste boucle sur elle meme sans chevaucher ptr
        if(l->suiv == LZL) break;
    }
//Fusion avec une ZL contigüe d'Avant ET d'Aprés 
    if (((void*)z==(void*)l+l->taille_mem) && ((void*)z+size==(void*)l->suiv) ) {
        if (l->suiv == LZL) {
            LZL = l;
        }
        l->taille_mem+=size+(l->suiv)->taille_mem;
        l->suiv=(l->suiv)->suiv;
        mem_lib += size;
        return 0;
    }
// Fusion avec la ZL contigüe d'avant
    if((void*)z==(void*)l+l->taille_mem) {
        l->taille_mem+=size;
        mem_lib += size;
        return 0;
    }
    // Fusion avec la ZL contigüe d'après
    if((void*)z+size==(void*)l->suiv) {
        z->taille_mem=size+(l->suiv)->taille_mem;
        z->suiv=(l->suiv)->suiv;
        l->suiv=z;
        mem_lib += size;
        return 0;
    }
// Cas ou la nouvelle ZL est entre deux ZO    
    z->taille_mem=size;
    z->suiv=l->suiv;
    l->suiv=z;
    mem_lib += size;
    return 0;
}

int mem_destroy()
{
    free(zone_memoire);
    zone_memoire = 0;
    LZL = 0;
    mem_lib = 0;
    return 0;
}

