## Guida 
# Installazione dei Software

Per Visual Studio
- Navigare su https://visualstudio.microsoft.com/downloads/ e scaricare Visual Studio 17 2022 Community
- Aperto l'installer di Visual Studio, selezionare i workload "C++ Desktop Application Development", "C++ Game Development"
- Aprire Visual Studio e installare il plugin "SonarLint" (opzionale)

Aprire il terminale e digitare i seguenti comandi
- winget install git
- winget install CMake

# Clonare la repository in locale

Navigare nella directory nella quale la repository deve essere contenuta e digitare 

- git clone https://github.com/alexoz12v2/cgProjectEngine.git

Dopodiche' entrare nella repository

- cd cgProjectEngine

# Setup Della Repository in Locale

Per buildare il progetto, digitare i seguenti comandi dalla project root directory

- mkdir build && cd build
- cmake .. -G "Visual Studio 17 2022"
- cmake --build . --config <Configurazione>

dove <Configurazione> una tra le configurazioni contenute nel file CMakePresets.json, ad esempio "Debug"

Dopo aver buildato il progetto la prima volta, sara' possibile farlo attraverso Visual Studio per le volte successive.
Ricordarsi, dopo aver buildato tutto, di spostare i necessari DLL/SO files nella cartella dell'eseguibile (build/testbed/Debug per Windows),
che si trovano in external/irrKlang/lib/${piattforma utilizzata}
