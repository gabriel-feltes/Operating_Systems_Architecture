//
//  sha256.hpp
//  t22
//
//  Created by Martin on /29/519.
//  Copyright © 2019 Martin. All rights reserved.
//

#ifndef sha256_hpp
#define sha256_hpp

#if OPENSSL_VERSION_NUMBER >= 0x3000000000L

#include <openssl/evp.h>

#else

#include <stdio.h>
#include <openssl/sha.h>
#include <openssl/bio.h>


#endif

#include <string>
std::string printSha256(const char *path);


#endif /* sha256_hpp */
