#include "common.h"

using std::string;

string ReadEnvironmentVariable(string name)
{
    char *env = getenv(name.c_str());
    if(env == NULL)
    {
        return string();
    }

    return string(env);
}
