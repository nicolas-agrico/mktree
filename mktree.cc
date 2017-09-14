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

int main(int argc, char *argv[])
{
  if (argc != 2)
    throw std::invalid_argument{"Pas de listing spécifié."};

  std::ifstream file{argv[1]};
  if (!file)
    throw std::runtime_error{"Impossible d'ouvrir le fichier."};

  int ret = mkdir("root", 0755);
  if (ret < 0 and errno != EEXIST) throw syscall_error{nullptr};
  int rootfd = open("root", O_DIRECTORY | O_PATH);
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