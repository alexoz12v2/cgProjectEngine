1.5, 1.math, 1.6,

1.1 coding style

# Memory Management System

La memoria dedicata al gioco va preallocata in fase di inizializzazione dell'applicazione e dopodiche gestita internamente, senza ulteriori chiamate al kernel, dall'applicazione 
stessa. Un sistema di memory management puo' diventare arbitrariamente molto complesso, Quindi cercheremo di limitare la complessita' optando per questa soluzione:

Una Huge Page di memoria (1GB) e' allocata allo startup dell'applicazione. Essa viene suddivisa in 4 slots da 256MB ciascuna. 
- Il Primo Slot e' assegnato alla gestione delle risorse interne del motore di gioco, persistenti per tutta la durata dell'applicazione (eg. moduli built-in, moduli shared...)
  Tale slot e' gestito con un Pool Allocator
- Il Secondo Slot e' assegnato ai dati transienti dell'i-esimo frame. Tale slot e' gestito con uno Stack Allocator
- Il Terzo Slot e' assegnato ai dati transienti dell'(i+1)-esimo frame. Tale slot e' gestito con uno Stack allocator
- Il Quarto Slot e' assegnato alle risorse piu' longeve (geometria del livello, textures, ...) gestito con uno Stack Allocator

## Funzionamento dei secondo e terzo slots come Double-Buffered Allocator
```
class DoubleBuffer_t
{
public:
    // ...
private:
    Byte_t *m_stack[2];
    U32_t   m_offset[2];
    U32_t   m_size;
    U8_t    m_curStack;
};
```
L'idea e' quella di avere uno slot per le memorie transienti dedicato a memorizzare tutte le risorse in input in un determinato frame, e un altro slot dedicato al 
memorizzare le risorse modificate/in output in quel frame. Cio' permette l'esecuzione di piu' Jobs in maniera parallela/asincrona senza aver paura che un thread intacchi il contenuto
dello slot in lettura. L'indice dello slot utilizzato in lettura per il frame corrente e' dato dalla parita' del frame index `frameParity`

Uno stack allocator e' gestito con uno stack, ovvero con un offset che ne indica il top of stack. Quando una risorsa viene "freed", l'offset viene fatto recedere solo se tale risorsa
e' l'ultima ad essere stata allocata. Altrimenti non fa nulla, ed infatti, tale operazione potrebbe anche non essere implementata data la natura transiente dei dati, e sarebbe 
sufficiente implementare una funzione `clear` che ripristina l'offset al suo stato iniziale. 
**Nota che in questo modo i destructors non sono invocati, dunque utilizza soltanto classi `trivially_destructible` all'interno di questo**

## Pool Allocator
Un pool allocator e' invece gestito in maniera differente. Esso e' diviso in due sezioni. La prima piccola sezione e' un indice, che (probabilmente gestibile con istruzioni SIMD?)
indica i vari offsets ai quali i pools sono stati collocati. Tale indice Occupera 8 MB (?) della totale struttura, lasciando 255 MB disponibile per le risorse interne del motore di 
gioco. L'idea e' quella di suddividere la porzione rimanente dello slot in buffers, ciascuno contenente blocchi free di memoria rispettivamente di 
- 64 bytes allineati in un 16 byte boundary, 
- 128 bytes allineati in un 16 byte boundary,
- 256 bytes allineati in un 16 byte boundary,
Piuttosto che dare una suddivisione statica in lista di blocchi da 64, lista di blocchi da 128, lista di blocchi da 256, la suddivisione viene fatta dinamicamnte in modo gerarchico
per raggiungere 8 blocchi della size richiesta. in questo modo:

1. controlla nell'indice se c'e gia un ottetto non full della size richiesta. Se c'e', guarda nel nodo corrispondente, leggi la posizione dell'ottetto e indice di primo blocco 
   libero, incrementalo.
2. Se non hai trovato il blocco, allora naviga nella regione best-fit (scannerizza tutta la gerarchia e trova la minima porzione di memoria che puo' contenere l'ottetto richiesto)
   (potenzialmente il passaggio piu lento)
3. Suddividi, aggiornando l'index, finche' non ottieni la minima regione che possa contenere l'ottetto, allinea l'ottetto ad un 16b yte boundary e marca la sezione con 
   first-free = 1. Ritorna il blocco.

L'index e' logicamente un Binary Search Tree. Esso lo implementiamo in un array, il quale e' 8 MB - 128 KB in size, sufficiente a indicizzare la rimanente porzione di 248 MB di 
memoria anche nel peggior caso possibile in cui il buffer e' pieno di ottetti da 64 bytes, size minima di allocazione
- Esponiamo allora questa regione di 128 KB come variabile inutilizzata chiamata la "Red Zone", la quale sara' una regione di memoria che una funzione puo' usare senza problemi 
  **finche' non fa un altra chiamata a funzione, costruttore, overloaded operator, in quanto anch'essi possono potenzialmente usare la Red Zone e sovrascriverla**

Ciascun nodo dell'index e' di 8 bytes, ed e' un `TaggedPointer`, ovvero un indirizzo di memoria nei cui 4 bit piu significativi, sempre a zero in un indirizzo valido, e' stato 
inserito l'indice `firstFree` + flag `full`

Lo spazio inutilizzato in ogni ottetto puo' essere utilizzato per taggare l'ottetto in qualche modo (?)

# Tracing Memory Allocations
Nelle debug builds, tutte le routine di allocazione a memoria devono aggiungere il supporto per tracciare tutte le memory allocation e deallocation che avvengono nel sistema, che 
magari possono essere scritte in un file di log `memfoot.txt`. Tale file e' reso disponibile in una macro di debug `CGEDBG_memTransaction`, che 

Ogni allocator, quando scrive un record su questo file testuale, deve presentare
- delimitatori, per rendere ciascuna chiamata distinguibile da un altra
- un header line dove descrive tipo di funzione (alloc, free, ...), quantita' richiesta, quantita effettivamenente allocata, timestamp, durata operazione (dipende dall'
  implementazione del concetto di tempo)
- stato del memory allocator prima e dopo l'allocazione
