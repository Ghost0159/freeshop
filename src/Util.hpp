#ifndef FREESHOP_UTIL_HPP
#define FREESHOP_UTIL_HPP

#include <sys/stat.h>
#include <string>
#include <cpp3ds/Network/Http.hpp>
#include <cpp3ds/System/String.hpp>
#ifndef EMULATION
#include <3ds.h>
#endif

namespace FreeShop
{

bool pathExists(const char* path, bool escape = true);
bool fileExists (const std::string& name);
void makeDirectory(const char *dir, mode_t mode = 0777);
int removeDirectory(const char *path, bool onlyIfEmpty = false);
std::string getCountryCode(int region);
uint32_t getTicketVersion(cpp3ds::Uint64 tid);
void hexToRGB(std::string hexValue, int *R, int *G, int *B);
std::string ReplaceAll(std::string str, const std::string& from, const std::string& to);
cpp3ds::String getUsername();

} // namespace FreeShop

#endif // FREESHOP_UTIL_HPP
