#ifndef LOGGING_H
#define LOGGING_H

#include <iostream>

// TODO: make (debug) logging optional with CLI
#define SCIS_DEBUG_MODE_ON

#ifdef SCIS_DEBUG_MODE_ON
  #define SCIS_DEBUG(D) (std::cerr << "[DEBUG] " << D << std::endl)
#else
  #define SCIS_DEBUG(D) (0)
#endif

#define SCIS_INFO(I)    (std::cerr << "[INFO] " << I << std::endl)
#define SCIS_WARNING(W) (std::cerr << "[WARNING] " << W << std::endl)
#define SCIS_ERROR(E)   {std::cerr << "[ERROR] " << E << std::endl << std::flush; abort();}(0)

#endif // LOGGING_H
