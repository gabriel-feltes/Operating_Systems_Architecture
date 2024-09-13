/**
 * NÃO MODIFIQUE ESTE ARQUIVO 
 */
 

#ifndef fs_h
#define fs_h
#include <string>

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome);

#endif /* fs_h */