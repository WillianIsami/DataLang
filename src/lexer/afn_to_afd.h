#ifndef AFN_TO_AFD_H
#define AFN_TO_AFD_H

#include "datalang_afn.h"

/*
 * Converte um AFN em um AFD equivalente usando o algoritmo de construção de subconjuntos.
 * 
 * @param afn O AFN a ser convertido
 * @return AFD* O AFD resultante da conversão, ou NULL em caso de erro
 */
AFD* afn_to_afd(AFN* afn);

/*
 * Exibe a tabela de transições do AFD para debug
 * 
 * @param afd O AFD cuja tabela será exibida
 */
void print_afd_table(AFD* afd);

#endif // AFN_TO_AFD_H