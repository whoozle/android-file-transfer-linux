#ifndef DEMANGLE_H
#define	DEMANGLE_H

#include <cxxabi.h>

namespace mtp
{
	inline std::string Demangle(const char *abiName)
	{
	  int status;    
	  char *ret = abi::__cxa_demangle(abiName, 0, 0, &status);  

	  std::string name(ret);
	  free(ret);
	  return name;
	}
}

#endif	/* DEMANGLE_H */

