## Guida 
# Installazione dei Software

Per Visual Studio, software che include un IDE, Generatore per CMake, Compilatore C++ MSVC, e Linker
- Navigare su https://visualstudio.microsoft.com/downloads/ e scaricare Visual Studio 17 2022 Community
- Aperto l'installer di Visual Studio, selezionare i workload "C++ Desktop Application Development", "C++ Game Development"
- Aprire Visual Studio e installare il plugin "SonarLint"

Aprire il terminale e digitare i seguenti comandi
- winget install git
- winget install CMake
- winget install Chocolatey

Aprire Powershell come amministratore e digitare i seguenti comandi
- choco install Doxygen -y
- choco install graphviz -y
- choco install cppcheck -y
- choco install ccache -y
- choco install clang-format -y
- choco install clang-tidy -y

Navigare nella directory nella quale la repository deve essere contenuta e digitare 

# Clonare la repository in locale

- git clone https://github.com/alexoz12v2/cgProjectEngine.git

Dopodiche' entrare nella repository

- cd cgProjectEngine

# Setup Della Repository in Locale

Per buildare il progetto, digitare i seguenti comandi dalla project root directory

- mkdir build && cd build
- cmake .. -G "Visual Studio 17 2022"
- cmake --build . --config <Configurazione>

dove <Configurazione> una tra le configurazioni contenute nel file CMakePresets.json, ad esempio "Debug"

## Struttura della Repository

## Creazione di un Modulo 

## Naming Conventions
