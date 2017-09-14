# Installation
Pour compiler, installez le paquet `build-essential` et tapez `make` dans le répertoire.

Pour utiliser un compilation différent de celui par défaut, utilisez la variable d'environnement `CXX`, par exemple:
```bash
env CXX=clang++ make
```

Ensuite, mettez le fichier `mktree` dans un endroit connu de votre shell, par exemple `/usr/local/bin`.

# Utilisation
Générez le fichier .lst avec la commande suivante, par exemple pour le dossier `/data`:
```bash
DIR=/data
find "$(realpath $DIR)" -type f > dir.lst
```

Puis ensuite utilisez la commande suivante:
```bash
mktree dir.lst
```

L'arborescence sera créée avec des faux fichiers dans le répertoire `root`.
