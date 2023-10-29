## Coding Conventions
Questo breve documento enumera tutte le regole di formattazione e stile di programmazione al fine di poter mantenere 
nel modo piu' efficiente possibile una codebase condivisa.

# Naming Conventions
* Tutti i nomi delle variabili sono in inglese, in *camelCase*
* Tutti i nomi dei tipi sono in inglese, in *PascalCase*. Inoltre
    * Se il tipo e' standard\_layout, trivial, allora ha il suffisso "_t"
    * Se il tipo e' un enum class, allora ha il prefisso "E", e tutti i suoi \
      valori hanno il prefisso "e"
    * Se il tipo non e' dinamicamente polimorfico, allora ha il suffisso "_s"
    * Se il tipo non ha membri e ha soltanto pure virtual member functions\
      allora ha il prefisso "I". Tale tipo puo' essere ereditato solo mediante 
      public inheritance
* Le indentazioni sono fatte mediante 4 spazi. Nessun TAB.
* La colonna massima in cui si puo' arrivare a scrivere prima di andare a \
  capo e' 100
* I membri privati di una classe hanno il prefisso "m_"
* le variabili globali, visibili al di fuori della translation unit in cui sono
  definite, hanno il prefisso "g_"

unica eccezione fatta per i tipi core di una data piattaforma, eg HWND, GLuint,
__mm256

# Style
* prima di codificare una qualsiasi funzionalita', un file.md omonimo nella \
  cartella docs/, nel quale descrivi il design delle tue classi/funzioni, 
  opzionalmente dettagliandoli con delle immagini
* Ogni funzione, classe, valore di un enum, membro, dichiarato o definito in un header file deve essere accompagnata
  dal suo commento doxygen, introdotto con
  ```
  /** @fn nome della funzione
   *  @brief breve descrizione della funzione
   *         puo' anche continuare piu' di una riga   
   * Descrizione dettagliata qui.
   * @param x descrizione parametro
   * @return descrizione oggetto ritornato
   */
  ```
  maggiori dettagli in [Doxygen Website](https://www.doxygen.nl/manual/docblocks.html#cppblock)
* Non utilizzare eccezioni. Infatti, il codice e' compilato con eccezioni disattivate, quindi crasheresti il programma. 
    come alternative, utilizza tl::optional, std::variant, oppure un enum class 
    come error code (controllare se error code gia dichiarato nel Core). (TO UPDATE)
* Non utilizzare std::string e altri standard containers come std::vector, std::map, ... eccetto std::array. Sono tassativi in termini di performance e memory hungry

* Dichiara funzioni che prendono al massimo 4 parametri di cui
    - 2 sono numeri interi da 64, pointers, references(questo include anche passare 1 struct con dentro 2 interi). 
    Nota che se li passi piu' piccoli (eg 32), sempre due sono il massimo, se ne vuoi di piu', usa bitwise ops per combinarli
    - 2 sono floating point numbers (o built-it floating register types)
