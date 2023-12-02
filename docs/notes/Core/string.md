# String Utilities

Per motivi di performance, __cgProjectEngine non utilizzera string classes__ come `std::string`, ma scriveremo e importeremo una serie di funzioni che ci permettono 
di manovrare stringhe in modo agevole.

Distinguiamo fondamentalmente tra due categorie di stringhe:
    * le stringhe usate a scopo di sviluppo, in codifica ASCII
    * le stringhe da mostrare a schermo, codificate con un character encoding dello standard Unicode

## Development Strings

sono essenzialmente utilizzate internamente per motivi di identificazione. Chiaramente non si puo' utilizzare funzioni come `strcmp` o `strcpy` in modo 
profuso.
Dato che le stringhe sono difficoltose da manovrare manualmente, utilizziamo l'algoritmo *CRC64* per convertirle in `U64_t` typedef'd a `Sid64_t`.
