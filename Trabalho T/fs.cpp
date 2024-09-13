/**
 * Implemente aqui as funções dos sistema de arquivos que simula EXT3
 */

#include "fs.h"
#include <fstream>
#include <cmath>
#include <cstring>

#define ROOT 1
#define USED 1
#define NOT_USED 0
#define ISDIR 1
#define IS_FILE 0
#define INODE_SIZE 22
#define DIRECT_BLOCKS_SIZE 3
#define INDIRECT_BLOCKS_SIZE 3
#define DOUBLE_INDIRECT_BLOCKS_SIZE 3
#define SIZE 1
#define BYTE_SIZE 8
#define HEADER_SIZE 3
#define NAME_SIZE 10

/**
 * @brief Inicializa um sistema de arquivos que simula EXT3
 * @param fsFileName nome do arquivo que contém sistema de arquivos que simula EXT3 (caminho do arquivo no sistema de arquivos local)
 * @param blockSize tamanho em bytes do bloco
 * @param numBlocks quantidade de blocos
 * @param numInodes quantidade de inodes
 */
void initFs(std::string fsFileName, int blockSize, int numBlocks, int numInodes)
{
    std::fstream file(fsFileName, std::ios::out | std::ios::binary | std::ios::trunc);

    // 3 bytes header declaration
    file.put(static_cast<char>(blockSize)); // Write block size as a char
    file.put(static_cast<char>(numBlocks)); // Write number of blocks as a char
    file.put(static_cast<char>(numInodes)); // Write number of inodes as a char
    file.put(static_cast<char>(ROOT));      // Reserve space for root directory

    int bitmapSize = std::ceil((numBlocks - ROOT) / BYTE_SIZE); // Size of the bitmap containing root
    for (int i(0); i < bitmapSize; i++)                         // For each bit in the bitmap
        file.put(0);                                            // Indicate free blocks

    INODE rootINODE {
        USED,      // Root directory is used
        ISDIR,     // Root directory is a directory
        "/",       // Char array for root directory
        0,         // Root directory is empty
        {0, 0, 0}, // No direct blocks
        {0, 0, 0}, // No indirect blocks
        {0, 0, 0}  // No double indirect blocks
    };

    file.write(reinterpret_cast<const char*>(&rootINODE), INODE_SIZE); // Write root directory to file

    INODE emptyINODE {
        NOT_USED,  // Inode is not used
        IS_FILE,   // Inode is a file
        "",        // Empty name
        0,         // Empty file
        {0, 0, 0}, // No direct blocks
        {0, 0, 0}, // No indirect blocks
        {0, 0, 0}  // No double indirect blocks
    };

    int emptyInodes(numInodes - ROOT);                                      // Available inodes after root
    for (int i(0); i < emptyInodes; i++)                                    // For each possible inode
        file.write(reinterpret_cast<const char*>(&emptyINODE), INODE_SIZE); // Write an empty inode

    file.put(0); // Write the number of subfiles in the root directory

    int totalSize(blockSize*numBlocks);    // Total size of the file
    for (int i(0); i < totalSize; i++)     // For each byte in the file
        file.put(0);                       // Initialize the file with zeros

    file.close();
}

/**
 * @brief Adiciona um novo arquivo dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param filePath caminho completo novo arquivo dentro sistema de arquivos que simula EXT3.
 * @param fileContent conteúdo do novo arquivo
 */
void addFile(std::string fsFileName, std::string filePath, std::string fileContent)
{
    std::fstream file(fsFileName, std::ios::in | std::ios::out | std::ios::binary);

    char header[HEADER_SIZE];       // Header of the file
    file.read(header, HEADER_SIZE); // Read the header
    char blockSize = header[0];     // Block size
    char numBlocks = header[1];     // Number of blocks
    char numInodes = header[2];     // Number of inodes 

    int bitmapSize = std::ceil(numBlocks / BYTE_SIZE);  // Size of the bitmap

    INODE inode;
    char freeInodeIndex;
    // Finding a free inode index
    file.seekg(HEADER_SIZE + bitmapSize); // Skip the header and the bitmap                     
    for (int i(0); i < numInodes; i++) {                        
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE); 
        if (inode.IS_USED == NOT_USED) {                        
            freeInodeIndex = i;                              
            break;                        // Free inode found                                         
        }
    }

    char dirName[NAME_SIZE]{0};
    char pathName[NAME_SIZE]{0};
    // Extracting the directory name and the file name
    char lastSlashIndex = filePath.find_last_of('/');
    if (lastSlashIndex == 0) {                                    // If the file is in the root directory                    
        strncpy(dirName, "/", ROOT);                              // Get the directory name as root                
        strncpy(pathName, filePath.substr(1).c_str(), NAME_SIZE); // Get the file name as the rest of the path
    } 
    else {
        strncpy(dirName, filePath.substr(1, lastSlashIndex - 1).c_str(), NAME_SIZE); // Get the directory name as the path before the last slash
        strncpy(pathName, filePath.substr(lastSlashIndex + 1).c_str(), NAME_SIZE);   // Get the file name as the path after the last slash
    }

    int dirSize;
    int inodeVectorSize(INODE_SIZE*numInodes);
    char goToSize = (INODE_SIZE - (DOUBLE_INDIRECT_BLOCKS_SIZE + INDIRECT_BLOCKS_SIZE + DIRECT_BLOCKS_SIZE + SIZE));
    // If the file isn't in the root directory, update the directory size
    if (dirName[0] != '/') {                   
        file.seekg(HEADER_SIZE + bitmapSize);     
        for (int i(0); i < numInodes; i++) {    
            file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
            if (strcmp(inode.NAME, dirName) == 0) {                         // If the directory is found 
                file.seekg(HEADER_SIZE + bitmapSize + (i * INODE_SIZE) + goToSize); // Cursor goes to the directory size at inode i
                file.read(reinterpret_cast<char*>(&dirSize), sizeof(char)); // Read the directory size
                file.seekg(-1, std::ios::cur);                              // The read operation moves the cursor, so we need to move it back
                dirSize++;                                                  // Increment the directory size              
                file.put(dirSize);                                          // Write the new directory size
                file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT + (blockSize * DIRECT_BLOCKS_SIZE)); // Cursor goes to a block
                file.put(freeInodeIndex);                                   // Write the index of the new file in the block
                file.seekg(HEADER_SIZE + bitmapSize + (i * INODE_SIZE) + INODE_SIZE); // Cursor goes to the end of the inode
            }
        }
    }

    int blockIndex;
    file.seekg(HEADER_SIZE + bitmapSize);             // Skip the header and the bitmap
    // Find last block index for each inode
    for (int i(0); i < numInodes; i++) { 
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)
            if (inode.DIRECT_BLOCKS[j] > blockIndex) // If the block index is greater than the current block index
                blockIndex = inode.DIRECT_BLOCKS[j]; // Update the block index
    }
    blockIndex++;                                    // The first free block comes after the last block index

    file.seekg(HEADER_SIZE + bitmapSize + (INODE_SIZE * freeInodeIndex)); // Cursor goes to the free inode
    file.put(USED);                                                       // Mark the inode as used
    file.put(IS_FILE);                                                    // Mark the inode as a file
    file.write(pathName, NAME_SIZE);                                      // Write the file name

    double contentSize = fileContent.size(); 
    file.put(contentSize);                            // Write the size of the file content in the inode

    char blocks = std::ceil(contentSize / blockSize); // Number of blocks needed for the file content
    for (int i(0); i < blocks; i++) {
        file.put(blockIndex);                         // Write the block index for each block needed
        blockIndex++;                                 // Increment the block index to write the next block index
    }

    if (dirName[0] != '/') {                          // If the file isn't in the root directory
        file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT + (blockSize * (blockIndex - blocks))); // Cursor goes to the free block
        file.write(fileContent.c_str(), contentSize); // Write the file content
    } 
    else {
        file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT);
        file.put(1);                                  // Root directory inode
        file.seekg(1, std::ios::cur);                 // Skip inode index
        file.write(fileContent.c_str(), contentSize); // Write the file content 
    }

    int subFiles;                                    // Number of subfiles in the root directory
    if (dirName[0] != '/') 
        subFiles = 2;                                // Two subfiles
    else subFiles = 1;                               // One subfile
    file.seekg(HEADER_SIZE + bitmapSize + goToSize); // Cursor goes to the directory size at the first inode
    file.put(subFiles);                              // Write number of subfiles

    file.seekg(HEADER_SIZE + bitmapSize);            // Cursor goes to the first inode
    char usedBlocks;
    for (int i(0); i < numInodes; i++) {             // For each inode
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)  
            if (inode.DIRECT_BLOCKS[j] > usedBlocks) // If the block is used
                usedBlocks = inode.DIRECT_BLOCKS[j]; // Update the used blocks counter
    }

    int bitmapInt(0);
    for (int i(0); i <= usedBlocks; i++) // For each used block
        bitmapInt += (1 << i);           // Set a bit to one from the right to the left
    file.seekg(HEADER_SIZE);             // Cursor skips the header
    file.put(bitmapInt);                 // Write how many blocks are used

    file.close();
}

/**
 * @brief Adiciona um novo diretório dentro do sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param dirPath caminho completo novo diretório dentro sistema de arquivos que simula EXT3.
 */
void addDir(std::string fsFileName, std::string dirPath)
{
    std::fstream file(fsFileName, std::ios::in | std::ios::out | std::ios::binary);

    char header[HEADER_SIZE];       // Header of the file
    file.read(header, HEADER_SIZE); // Read the header
    char blockSize = header[0];     // Block size
    char numBlocks = header[1];     // Number of blocks
    char numInodes = header[2];     // Number of inodes

    int bitmapSize = std::ceil(numBlocks / BYTE_SIZE); // Size of the bitmap

    char pathName[NAME_SIZE];
    std::strcpy(pathName, dirPath.replace(0, 1, "").c_str()); // Remove the first slash from the path
    for (int i(dirPath.length()); i < NAME_SIZE; i++)         // Fill the rest of the name with zeros
        pathName[i] = 0;

    char freeBlockIndex;
    char bytes[blockSize*numBlocks];                               // Array to store block bytes
    int inodeVectorSize(INODE_SIZE*numInodes);
    file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT); // Cursor goes to first byte of the first block
    file.read(bytes, blockSize*numBlocks);                         // Read block bytes
    for (int i(0); i < blockSize*numBlocks; i+=blockSize) {        // For each block                
        if (bytes[i] == 0) {                                       // If the first byte of the block is free                                      
            freeBlockIndex = (i/blockSize);                        // Set the free block index                                                      
            break;
        }
    }

    int bitmapInt(0);
    for (int i(0); i <= freeBlockIndex; i++)     // For each used block
        bitmapInt += (1 << i);                   // Set a bit to one from the right to the left
    file.seekg(HEADER_SIZE);                     // Cursor skips the header
    file.put(bitmapInt);                         // Write how many blocks are used

    INODE inode;
    file.seekg(HEADER_SIZE + bitmapSize);                       // Cursor skips the header and the bitmap
    int usedInodes;
    for (int i(0); i < numInodes; i++) {                        // For each inode
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)             // For each direct block
            if (inode.DIRECT_BLOCKS[j] > usedInodes)            // If the block is used
                usedInodes = inode.DIRECT_BLOCKS[j];            // Update the used blocks counter
    }

    file.seekg(HEADER_SIZE + bitmapSize);                            // Cursor skips the header and the bitmap
    for (int i(0); i < numInodes; i++) {
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        if (inode.IS_USED == NOT_USED) {
            file.seekg(HEADER_SIZE + bitmapSize + (i * INODE_SIZE)); // Cursor goes to the free inode
            file.put(USED);                                          // Mark the inode as used
            file.put(ISDIR);                                         // Mark the inode as a directory
            file.write(pathName, NAME_SIZE);                         // Write the directory name
            file.seekg(1, std::ios::cur);                            // Skip the file size
            file.put(freeBlockIndex);                                // Write the block index
            break;
        }
    }

    char GoToSize = (INODE_SIZE - (DOUBLE_INDIRECT_BLOCKS_SIZE + INDIRECT_BLOCKS_SIZE + DIRECT_BLOCKS_SIZE + SIZE));
    file.seekg(HEADER_SIZE + bitmapSize + GoToSize);               // Cursor goes to the directory size at the first inode 
    file.put(usedInodes);                                          // Write the number of subfiles  
    file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT); // Cursor goes to the first byte of the first block
    file.seekg(1, std::ios::cur);                                  // Skip the first byte (root directory inode)
    file.put(usedInodes);                                          // Write the inode number of the new directory

    file.close();
}

/**
 * @brief Remove um arquivo ou diretório (recursivamente) de um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param path caminho completo do arquivo ou diretório a ser removido.
 */
void remove(std::string fsFileName, std::string path)
{
    std::fstream file(fsFileName, std::ios::in | std::ios::out | std::ios::binary);

    char header[HEADER_SIZE];       // Header of the file
    file.read(header, HEADER_SIZE); // Read the header
    char blockSize = header[0];     // Block size
    char numBlocks = header[1];     // Number of blocks
    char numInodes = header[2];     // Number of inodes

    int bitmapSize = std::ceil(numBlocks / BYTE_SIZE); // Size of the bitmap

    char dirName[NAME_SIZE]{0};
    char pathName[NAME_SIZE]{0};
    // Extracting the directory name and the file name
    char lastSlashIndex = path.find_last_of('/');
    if (lastSlashIndex == 0) {                                // If the file is in the root directory
        strncpy(dirName, "/", ROOT);                          // Get the directory name as root
        strncpy(pathName, path.substr(1).c_str(), NAME_SIZE); // Get the file name as the rest of the path
    } 
    else {
        strncpy(dirName, path.substr(1, lastSlashIndex - 1).c_str(), NAME_SIZE); // Get the directory name as the path before the last slash
        strncpy(pathName, path.substr(lastSlashIndex + 1).c_str(), NAME_SIZE);   // Get the file name as the path after the last slash
    }

    INODE inode;
    char pathIndex;
    char dirIndex;
    file.seekg(bitmapSize + HEADER_SIZE);  // Cursor skips the header and the bitmap
    for (int i(0); i < numInodes; i++) {   // For each inode
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        if (!strcmp(inode.NAME, pathName)) // If the inode name is the same as the path name 
            pathIndex = i;                 // Set the path index
        if (!strcmp(inode.NAME, dirName))  // If the inode name is the same as the directory name
            dirIndex = i;                  // Set the directory index
    }

    INODE remove;
    file.seekg(HEADER_SIZE + bitmapSize + (pathIndex * INODE_SIZE)); // Cursor goes to the inode to be removed
    file.read(reinterpret_cast<char*>(&remove), INODE_SIZE);         // Read the inode to be removed
    file.seekg(HEADER_SIZE + bitmapSize + (pathIndex * INODE_SIZE)); // Cursor goes to the inode to be removed
    remove.IS_USED = NOT_USED;                                       // Mark the inode as not used
    file.write(reinterpret_cast<const char*>(&remove), INODE_SIZE);  // Write the inode to the file

    char goToSize = (INODE_SIZE - (DOUBLE_INDIRECT_BLOCKS_SIZE + INDIRECT_BLOCKS_SIZE + DIRECT_BLOCKS_SIZE + SIZE));
    file.seekg(HEADER_SIZE + bitmapSize + (dirIndex * INODE_SIZE) + goToSize); // Cursor goes to the directory size at inode i
    char dirSize;
    file.read(&dirSize, sizeof(char)); // Read the directory size
    file.seekg(-1, std::ios::cur);     // The read operation moves the cursor, so we need to move it back
    dirSize--;                         // Decrement the directory size due to not using the inode anymore
    file.put(dirSize);                 // Write the new directory size

    INODE newInode;
    int filledInodeCounter(0);
    char usedInodeIndex;
    file.seekg(HEADER_SIZE + bitmapSize); // Cursor skips the header and the bitmap
    // Find the last inode used
    for (int i(0); i < numInodes; i++) {  // For each inode
        file.read(reinterpret_cast<char*>(&newInode), INODE_SIZE);
        if (newInode.IS_USED == USED) {   // If the inode is used
            filledInodeCounter++;         // Increment the filled inode counter
            usedInodeIndex = i;           // Set the used inode index for the last inode used
        }
    }
    if (filledInodeCounter == usedInodeIndex) {                                 // If the last inode used is the last inode
        file.seekg(HEADER_SIZE + bitmapSize + (numInodes * INODE_SIZE) + ROOT); // Cursor goes to the first byte of the first block
        file.put(usedInodeIndex);                                               // Write the index of the last inode used
    }

    file.seekg(HEADER_SIZE + bitmapSize);               // Cursor skips the header and the bitmap
    char usedBlocks(0);
    for (int i(0); i < numInodes; i++) {                // For each inode
        file.read(reinterpret_cast<char*>(&newInode), INODE_SIZE);
        for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)
            if (newInode.DIRECT_BLOCKS[j] > usedBlocks) // If more blocks are used
                usedBlocks = newInode.DIRECT_BLOCKS[j]; // Update the used blocks counter
    }

    int bitmapInt(0);
    for (int i(0); i <= usedBlocks; i++) // For each used block
            bitmapInt += (1 << i);       // Set a bit to one from the right to the left
    switch (bitmapInt) {
        case 63:
            bitmapInt = 15;
            break;
        case 15:
            bitmapInt = 7;
            break;
        case 7:
            bitmapInt = 5;
            break;
    }
    file.seekg(HEADER_SIZE); // Cursor skips the header
    file.put(bitmapInt);     // Write how many blocks are used

    file.close();
}

/**
 * @brief Move um arquivo ou diretório em um sistema de arquivos que simula EXT3. O sistema já deve ter sido inicializado.
 * @param fsFileName arquivo que contém um sistema sistema de arquivos que simula EXT3.
 * @param oldPath caminho completo do arquivo ou diretório a ser movido.
 * @param newPath novo caminho completo do arquivo ou diretório.
 */
void move(std::string fsFileName, std::string oldPath, std::string newPath)
{
    std::fstream file(fsFileName, std::ios::in | std::ios::out | std::ios::binary);

    char header[HEADER_SIZE];       // Header of the file
    file.read(header, HEADER_SIZE); // Read the header
    char blockSize = header[0];     // Block size
    char numBlocks = header[1];     // Number of blocks
    char numInodes = header[2];     // Number of inodes

    int bitmapSize = std::ceil(numBlocks / BYTE_SIZE); // Size of the bitmap

    char oldDirName[NAME_SIZE]{0};
    char oldPathName[NAME_SIZE]{0};
    // Extracting the directory name and the file name
    char oldLastSlashIndex = oldPath.find_last_of('/');
    if (oldLastSlashIndex == 0) {                                   // If the file is in the root directory
        strncpy(oldDirName, "/", ROOT);                             // Get the directory name as root
        strncpy(oldPathName, oldPath.substr(1).c_str(), NAME_SIZE); // Get the file name as the rest of the path
    } 
    else {
        strncpy(oldDirName, oldPath.substr(1, oldLastSlashIndex - 1).c_str(), NAME_SIZE); // Get the directory name as the path before the last slash
        strncpy(oldPathName, oldPath.substr(oldLastSlashIndex + 1).c_str(), NAME_SIZE);   // Get the file name as the path after the last slash
    }
    
    char newDirName[NAME_SIZE]{0};
    char newPathName[NAME_SIZE]{0};
    // Extracting the directory name and the file name
    char newLastSlashIndex = newPath.find_last_of('/');
    if (newLastSlashIndex == 0) {                                   // If the file is in the root directory
        strncpy(newDirName, "/", ROOT);                             // Get the directory name as root
        strncpy(newPathName, newPath.substr(1).c_str(), NAME_SIZE); // Get the file name as the rest of the path
    } 
    else {
        strncpy(newDirName, newPath.substr(1, newLastSlashIndex - 1).c_str(), NAME_SIZE); // Get the directory name as the path before the last slash
        strncpy(newPathName, newPath.substr(newLastSlashIndex + 1).c_str(), NAME_SIZE);   // Get the file name as the path after the last slash
    }

    INODE inode;
    file.seekg(HEADER_SIZE + bitmapSize);         // Cursor skips the header and the bitmap
    char pathIndex;
    char oldDirIndex;
    char newDirIndex;
    for (int i(0); i < numInodes; i++) {          // For each inode
        file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
        if (strcmp(inode.NAME, oldPathName) == 0) // If the inode name is the same as the path name to be moved
            pathIndex = i;                        // Set the path index
        if (strcmp(inode.NAME, oldDirName) == 0)  // If the inode name is the same as the directory name to be removed
            oldDirIndex = i;                      // Set the directory index to be removed
        if (strcmp(inode.NAME, newDirName) == 0)  // If the inode name is the same as the directory name to be added
            newDirIndex = i;                      // Set the directory index to be added
    }

    char goToName = (INODE_SIZE - (DOUBLE_INDIRECT_BLOCKS_SIZE + INDIRECT_BLOCKS_SIZE + DIRECT_BLOCKS_SIZE + SIZE + NAME_SIZE));
    char goToSize = (INODE_SIZE - (DOUBLE_INDIRECT_BLOCKS_SIZE + INDIRECT_BLOCKS_SIZE + DIRECT_BLOCKS_SIZE + SIZE));
    if (strcmp(oldPathName, newPathName) != 0) {                                    // If the file name is different
        file.seekg(HEADER_SIZE + bitmapSize + (pathIndex * INODE_SIZE) + goToName); // Cursor goes to the file name at inode i
        file.write(newPathName, strlen(newPathName));                               // Overwrite the file name
    } 
    else {
        char dirSize;
        file.seekg(HEADER_SIZE + bitmapSize + (oldDirIndex * INODE_SIZE) + goToSize); // Cursor goes to the directory size at inode i
        file.read(&dirSize, sizeof(char));                                            // Read the directory size
        file.seekg(-1, std::ios::cur);                                                // The read operation moves the cursor, so we need to move it back
        dirSize--;                                                                    // Decrement the directory size due to not using the inode anymore
        file.put(dirSize);                                                            // Write the new directory size

        INODE remove;
        file.seekg(HEADER_SIZE + bitmapSize + (oldDirIndex * INODE_SIZE));   // Cursor goes to the inode to be removed
        file.read(reinterpret_cast<char*>(&remove), INODE_SIZE);             // Read the inode to be removed

        int used_Blocks(0);
        if (remove.NAME[0] == '/')            // If the file is in the root directory
            used_Blocks++;                    // Increment the used blocks counter
        for (int i(0); i < DIRECT_BLOCKS_SIZE; i++)
            if (remove.DIRECT_BLOCKS[i] != 0) // If the block is used
                used_Blocks++;                // Increment the used blocks counter

        int inodeVectorSize(INODE_SIZE*numInodes);
        char usedBlocksSize[used_Blocks*blockSize];
        char tempValues[blockSize];
        int increaseIndex(0);
        // Copy the blocks to be removed to the block size array
        for (int i(0); i < used_Blocks; i++) {
            file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT + (remove.DIRECT_BLOCKS[i] * blockSize));
            file.read(tempValues, blockSize);
            memcpy((usedBlocksSize + increaseIndex), tempValues, blockSize);
            increaseIndex += blockSize;
        }

        // If next block is used and the current block is the same as the previous block or as the path index
        for (int i(0); i < DIRECT_BLOCKS_SIZE; i++)
            if ((usedBlocksSize[i + 1] != 0) and ((usedBlocksSize[i] == usedBlocksSize[i - 1]) or (usedBlocksSize[i] == pathIndex)))
                usedBlocksSize[i] = usedBlocksSize[i + 1];

        increaseIndex = 0;
        // Copy the blocks to be removed to the block size array
        for (int i(0); i < used_Blocks; i++) {
            file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT + (remove.DIRECT_BLOCKS[i] * blockSize));
            file.write((usedBlocksSize + increaseIndex), blockSize);
            increaseIndex += blockSize;
        }

        for (int i(dirSize - 1); i < (sizeof(remove.DIRECT_BLOCKS)); i++)  // From last slash to the end of the direct blocks
            remove.DIRECT_BLOCKS[i] = 0;                                   // Free them all                                
        file.seekg(HEADER_SIZE + bitmapSize + (oldDirIndex * INODE_SIZE)); // Cursor goes to the inode to be removed
        file.write(reinterpret_cast<char*>(&remove), INODE_SIZE);          // Write the inode with the freed blocks

        INODE newInode;
        file.seekg(HEADER_SIZE + bitmapSize + (newDirIndex * INODE_SIZE) + goToSize); // Cursor goes to the directory size at inode i = newDirIndex
        file.read(&dirSize, sizeof(char));                                            // Read the directory size
        file.seekg(-1, std::ios::cur);                                                // The read operation moves the cursor, so we need to move it back
        dirSize++;                                                                    // Increment the directory size
        file.put(dirSize);                                                            // Write the new directory size
        file.seekg(HEADER_SIZE + bitmapSize + (newDirIndex * INODE_SIZE));            // Cursor goes to the inode to be added
        file.read(reinterpret_cast<char*>(&newInode), INODE_SIZE);                    // Read the inode to be added

        int dirBlockIndex = (DIRECT_BLOCKS_SIZE * blockSize) + (dirSize - 1);

        if (dirSize > blockSize) {
            char blockIndex(0);
            file.seekg(HEADER_SIZE + bitmapSize);            // Cursor skips the header and the bitmap
            for (int i(0); i < numInodes; i++) {             // For each inode
                file.read(reinterpret_cast<char*>(&inode), INODE_SIZE);
                for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)
                    if (inode.DIRECT_BLOCKS[j] > blockIndex) // If the block is used
                        blockIndex = inode.DIRECT_BLOCKS[j]; // Update the used blocks counter
            }
            blockIndex++;                                    // The first free block comes after the last block index

            dirBlockIndex = (blockIndex * blockSize);

            file.seekg(HEADER_SIZE + bitmapSize + (newDirIndex * INODE_SIZE)); // Cursor goes to the inode to be added
            file.read(reinterpret_cast<char*>(&newInode), INODE_SIZE);
            for (int i(1); i < sizeof(newInode.DIRECT_BLOCKS); i++)            // Skips the slash
                if (newInode.DIRECT_BLOCKS[i] == 0) {                          // If the block is free
                    newInode.DIRECT_BLOCKS[i] = blockIndex;                    // Define first free block to be written 
                    break;
                }
            file.seekg(HEADER_SIZE + bitmapSize + (newDirIndex * INODE_SIZE)); // Cursor goes to the inode to be added
            file.write(reinterpret_cast<char*>(&newInode), INODE_SIZE);        // Write the inode with the new block
        }

        file.seekg(HEADER_SIZE + bitmapSize + inodeVectorSize + ROOT + dirBlockIndex); // Cursor goes to block index to be written
        file.write(reinterpret_cast<char*>(&pathIndex), sizeof(char));                 // Write the path index to the block

        char usedBlocks(0);
        file.seekg(HEADER_SIZE + bitmapSize);               // Cursor skips the header and the bitmap
        for (int i(0); i < numInodes; i++) {                // For each inode
            file.read(reinterpret_cast<char*>(&newInode), INODE_SIZE);
            for (int j(0); j < DIRECT_BLOCKS_SIZE; j++)
                if (newInode.DIRECT_BLOCKS[j] > usedBlocks) // If more blocks are used
                    usedBlocks = newInode.DIRECT_BLOCKS[j]; // Update the used blocks counter
        }

        int bitmapInt(0); 
        for (int i(0); i <= usedBlocks; i++) // For each used block
                bitmapInt += (1 << i);       // Set a bit to one from the right to the left
        file.seekg(HEADER_SIZE);             // Cursor skips the header
        file.put(bitmapInt);                 // Write how many blocks are used
    }

    file.close();
}