// 

#include "rswig.h"

bool RInitSystem()
{
   return pkgInitConfig(*_config) && pkgInitSystem(*_config,_system);

}
