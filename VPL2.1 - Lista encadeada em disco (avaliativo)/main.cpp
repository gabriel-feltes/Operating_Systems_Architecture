#include "gtest/gtest.h"
#include "fs.h"
#include "sha256.h"

#include <fstream>
#include <stdio.h>

void duplicate(std::string fsrc, std::string fdest)
{
    std::ifstream  src(fsrc, std::ios::binary);
    std::ofstream  dst(fdest, std::ios::binary);

    dst << src.rdbuf();
}


TEST(FsTest, case1){
    
    duplicate("lista.bin", "lista.bin.solucao");
    
    adiciona("lista.bin.solucao", "Anderson", "Everton");
    
    ASSERT_EQ(printSha256("lista.bin.solucao"),std::string("0A:F1:8F:E8:A6:34:9B:73:68:B1:52:66:74:71:DA:56:B3:6E:F8:96:0A:C4:F8:75:E1:C0:F1:DE:F4:5B:91:5D"));

}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
