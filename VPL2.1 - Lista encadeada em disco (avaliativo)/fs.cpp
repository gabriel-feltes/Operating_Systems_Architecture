#include "fs.h"
#include <fstream>

#define BYTE_NUM 4
#define NAME_SIZE 20
#define REG_SIZE 24

/**
 * @param arquivoDaLista nome do arquivo em disco que contem a lista encadeada
 * @param novoNome nome a ser adicionado apos depoisDesteNome
 * @param depoisDesteNome um nome presente na lista
 */
void adiciona(std::string arquivoDaLista, std::string novoNome, std::string depoisDesteNome)
{
    std::fstream file(arquivoDaLista, std::ios::in | std::ios::out | std::ios::binary);
    
    auto readIntFromFile = [](std::fstream &file) -> int {
        char bytes[BYTE_NUM];                      // Array to hold the 4 bytes read from the file
        file.read(bytes, BYTE_NUM);                // Read 4 bytes from the file into the array
        return (static_cast<int>(bytes[0]) <<  0 | // First byte  (least significant)
                static_cast<int>(bytes[1]) <<  8 | // Second byte (shifted by 8 bits)
                static_cast<int>(bytes[2]) << 16 | // Third byte  (shifted by 16 bits)
                static_cast<int>(bytes[3]) << 24); // Fourth byte (shifted by 24 bits)
    };
                       
    char start = readIntFromFile(file);            

    int newAddress(sizeof(int));                   
    file.seekg(newAddress);                        
    file.write("\1\0\0\0", BYTE_NUM);                 

    char name[NAME_SIZE]{0};                       // Array to hold the name
    for (int i(0); i < novoNome.size(); i++)       // Loop through the name    
        name[i] = novoNome[i];                     // Copy the name to the array
    file.write(name, NAME_SIZE);                   // Write the name to the file

    auto writeIntToFile = [](std::fstream &file, int value) {
        char bytes[BYTE_NUM];                      // Array to hold the 4 bytes of the integer
        bytes[0] = (value & 0x000000FF) >>  0;     // Extract the least significant byte
        bytes[1] = (value & 0x0000FF00) >>  8;     // Extract the second byte
        bytes[2] = (value & 0x00FF0000) >> 16;     // Extract the third byte
        bytes[3] = (value & 0xFF000000) >> 24;     // Extract the most significant byte
        file.write(bytes, BYTE_NUM);               // Write the 4 bytes to the stream
    };

    writeIntToFile(file, 0);                       

    bool nameFound;
    int foundRegisterAddress = start;              
    while (!nameFound) {                           // Loop until the name is found
        bool state = readIntFromFile(file);        
        file.read(name, NAME_SIZE);                // Read the name of the register         
        if (name == depoisDesteNome)               // If the name is the one we are looking for
            nameFound = true;                      // Break the loop            
        else {
            int next(readIntFromFile(file));
            file.seekg(next);                      // Move to the next register
            foundRegisterAddress = next;           // Update the address of the found register
        }
    }

    file.seekg(foundRegisterAddress + REG_SIZE);
    int addr = readIntFromFile(file);
    file.seekg(foundRegisterAddress + REG_SIZE);
    writeIntToFile(file, newAddress);
    file.seekg(newAddress + REG_SIZE);
    writeIntToFile(file, addr);

    file.close();
}