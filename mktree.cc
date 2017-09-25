/*
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <err.h>
#include <cstring>
#include <cerrno>

#include <iostream>
#include <fstream>
#include <string>
#include <stdexcept>

using namespace std::literals::string_literals;

class syscall_error : public std::runtime_error
{
public:
  using std::runtime_error::runtime_error;

  virtual const char *what() const noexcept override
  {
    const char *orig = this->std::runtime_error::what();
    const char *expl = strerror(error);

    if (!orig)
      return expl;

    else
    {
      const auto& s = orig + ": "s + expl;
      char *str = new char[s.size()+1];
      std::strcpy(str, s.c_str());
      return str;
    }
  }

public:
  const int error = errno;
};

int main(int argc, char *argv[], char *envp[])
{
  std::string rootpath;

  for (char **env = envp; *env; env++)
  {
    std::string line{*env};
    auto eqndx = line.find_first_of('=');
    if (eqndx == std::string::npos)
    {
      std::clog << "Mauvaise variable d'environnement " << line << std::endl;
      continue;
    }

    const auto& name = line.substr(0, eqndx);
    const auto& value = line.substr(eqndx+1, std::string::npos);

    if (name == "ROOT"s)
    {
      rootpath = value;
      break;
    }
  }
  if (rootpath.empty())
    rootpath = "root"s;

  if (argc != 2)
    throw std::invalid_argument{"Pas de listing spécifié."};

  errno = 0;
  std::ifstream file{argv[1]};
  if (!file)
    throw syscall_error{"Impossible d'ouvrir le fichier."};

  int ret = mkdir(rootpath.c_str(), 0755);
  if (ret < 0 and errno != EEXIST) throw syscall_error{nullptr};
  int rootfd = open(rootpath.c_str(), O_DIRECTORY | O_PATH);
  if (rootfd < 0) throw syscall_error{"Impossible d'ouvrir le dossier racine"};

  std::string filename;
  while (std::getline(file, filename))
  {
    // create intermediary directories
    if (filename.at(0) != '/')
      throw std::invalid_argument{"Chemins relatifs interdits: "s + filename};

    int lastfd = dup(rootfd);
    if (lastfd < 0) throw syscall_error{"Impossible de dupliquer le dossier racine"};
    std::string dir;
    bool broke = false;
    for (std::string::size_type i = 0, last = 1;; last = i+1)
    {
      i = filename.find_first_of('/', last);
      dir = filename.substr(last, i-last);
      if (i == std::string::npos)
        break;

      ret = mkdirat(lastfd, dir.c_str(), 0755);
      if (ret < 0 and errno != EEXIST)
        throw syscall_error{"Impossible de créer "s + dir};

      int fd = openat(lastfd, dir.c_str(), O_DIRECTORY | O_PATH);
      if (fd < 0)
      {
        warn("Impossible de créer l'un des dossiers du chemin de %s", filename.c_str());
        broke = true;
        break;
      }
      close(lastfd);
      lastfd = fd;
    }
    if (broke)
      continue;

    int fd = openat(lastfd, dir.c_str(), O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fd < 0) warn("Impossible de créer le fichier %s", filename.c_str());
    else
    {
      ret = write(fd, "lol\n", 3);
      if (ret < 0) throw syscall_error{"Impossible d'écrire le fichier "s + filename};
    }

    close(fd);
    close(lastfd);
  }

  return 0;
}
