#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>

#define SCIS_INFO(I)    (std::cout << "[INFO] " << I << std::endl)
#define SCIS_WARNING(W) (std::cerr << "[WARNING] " << W << std::endl)
#define SCIS_ERROR(E)   (std::cerr << "[ERROR] " << E << std::endl)

#endif // LOGGING_H
