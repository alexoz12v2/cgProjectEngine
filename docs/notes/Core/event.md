tutti gli esempi sono presi da Game Engine Architecture
# Event System

Una moltitudine di tipologie di eventi devono essere gestiti dal motore di gioco e propagati su tutti i moduli registrati per la ricezione di un certo
evento. Ci sono sia eventi provenienti dal sistema operativo in maniera asincrona, eg pressione di un tasto, che deve essere gestito con l'inserimento 
del tasto in un buffer con gestione differita, e eventi discreti dichiarati nella logica di gioco. Essi sono unificati sotto un unico modulo di 
gestione per avere una interfaccia unificata all'interazione tra i game objects

## Gestione del tempo di gioco

## Update dei game objects

ciascuno dei game objects avra' una funzione chiamata 1 volta per frame, che permette a tale object di eseguire la sua logica
```
virtual void update(float dt);
```
un approccio esemplificativo del genere non e' efficiente, in quanto non permette lo sharing di computazione tra game objects, e quindi la gestione 
di dipendenze tra sistemi. Tipicamente, un motore di gioco, nel suo main loop copre diverse fasi
```
while (true)
{
    // ...

    for (auto& obj : g_pool.view(EPoolTag_t::eGameObject)) 
    {  
        obj.preAnimUpdate(dt); // first update before animation blending (non ce lo abbiamo)
    }

    g_animationEngine.calculateIntermediatePoses(dt);   // questi 3 non sono presenti nei nostri

    for (auto& obj : g_pool.view(EPoolTag_t::eGameObject)) 
    {  
        obj.postAnimUpdate(dt); // first update before animation blending (non ce lo abbiamo)
    }

    g_ragdollSystem.applySkeletonsToRagDolls();         // sistemi, elencati a scopo di
    g_physicsEngine.simulate(dt);                       // esempio
    g_collisionEngine.detectAndResolveCollisions(dt);
    g_animationEngine.finalizePoseAndMatrixPalette();
    
    for (auto& obj : g_pool.view(EPoolTag_t::eGameObject)) 
    {  
        obj.finalAnimUpdate(dt); // first update before animation blending (non ce lo abbiamo)
    }
}
```
chiaramente questo esemmpio non conta quali sono gli oggetti registrati agli eventi preAnimUpdate, postAnimUpdate, finalAnimUpdate.
Inoltre, non conta la possibilita' di interdipendenze di oggetti o di suppportare tipologie di update diverse da un game object. 
Per colmare il supporto per le interdipendenze, nota che ciascun metodo di update di un oggetto puo dipendere dal corrispettivo metodo di update di un 
altro oggetto. Tali dipendenze possono essere visualizzate come un Fibonacci Heap.
Tutte le radici, tutti i figli di stessa depth possono essere raggruppati in buckets, e possiamo updateare ogni update singolarmente
```
enum class EBucket { eVehiclePlatforms, eCharacters, eAttachedObjects, eCount };
void updateBucket(EBucket_t bucket)
{
    // ...
    for (auto& gameObj : view(bucket))
    {
        gameObject.preAnimUpdate(dt);
    }
    // ...
    g_animationEngine.calculateIntermediatePoses(bucket, dt);
    
    for (auto& gameObj : view(bucket))
    {
        gameObject.postAnimUpdate(dt);
    }

    g_ragdollSystem.applySkeletonsToRagDolls(bucket);
    g_physicsEngine.simulate(bucket, dt);
    g_collisionEngine.detectAndResolveCollisions(bucket, dt);
    g_ragdollSystem.applyRagDolsToSkeletons(bucket);
    g_animationEngine.finalizePoseAndMatrixPalette(bucket);

    for (auto& gameObj : view(bucket))
    {
        gameObj.finalUpdate(dt);
    }
    // ...
}

void gameLoop() 
{
    while (true)
    {
        // ...
        updateBucket(EBucket_t::eVehiclePlatforms);
        updateBucket(EBucket_t::eCharacters);
        updateBucket(EBucket_t::eAttachedObjects);
        // ... 

        g_renderingEngine.renderSceneAndSwapBuffers();
    }
}
```
Il problema nell'implementazione delle dipendenze e' la *consistenza dello stato degli oggetti*. Ogni update e' una transazione per un oggetto che lo 
aggiorna da stato A a stato B. Essa deve essere *atomica*.
Dunque, per far si che cio' avvenga, game objects che richiedono lo *state vector* di un altro game objects devono specificare se vogliono lo state 
vector del frame precedente o quello aggiornato. 
Cio' vuol dire che bisogna *cachare stato precedente e corrente* per ogni game object (e i suoi components)

## Eventi come Message Passing

Diverse componenti del motore di gioco, il sistema operativo, e i game objects stessi sono al contempo EventEmitters e EventListeners. Cio' vuol dire 
che, eventi a vario livello di astrazione sono generati ed emessi nel sistema, e sono distribuiti a tutte le entita' in ascolto per quel determinato
evento.
La __function signature__ di ogni evento non e' nota a priori, cioe' gli event handler sfruttano la **dynamically typed late function binding**.
La tipologia di evento sara' identificata da uno *String Identifier*, descritto da `string.md`.
I dati trasportati da ciascun evento si riducono a 2 interi da 64 bits e due registri xmm da 128 bits, in accordo con il piu' stringente subset delle 
x86\_64 calling conventions dei vari sistemi operativi
```
union EventIntArg_t 
{
    Byte_t* p;
    U64_t   u64;
    I64_t   i64;
    // ...
};
using EventSignature_t = void(*)(EventIntArg_t idata0, EventIntArg_t idata1, V128f_t fdata0, V128f_t fdata1);
};
```
Questo approccio e' efficiente dal punto di vista dell'hardware, ma complesso dal punto di vista dell'utilizzo. Quindi in alternativa, forniamo la 
possibilita' di utilizzare un array di key-value pairs per eventi piu' complessi. Abbiamo scelto un array piuttosto che una struttura dati piu' 
complessa quale hashmap o binary search tree perche' non dovrebbero esserci liste troppo lunghe di argomenti, e negli event handlers molto 
probabilmente sono utilizzati tutti gli argomenti.
```
inline U32_t constexpr CGE_EVENT_MAX_COMPLEX_ARGS = 8;
struct EventComplexArg_t
{
public:
    struct Pair_t { Sid64_t key; U64_t value; };
    // ...
    
private:
    std::array<Pair_t, CGE_EVENT_MAX_COMPLEX_ARGS> m_cArgs;
}

union EventIntArg_t 
{
    EventComplexArg_t* c;
    Byte_t*            p;
    U64_t              u64;
    I64_t              i64;
    // ...
};
using EventSignature_t = void(*)(EventIntArg_t idata0, EventIntArg_t idata1, V128f_t fdata0, V128f_t fdata1);
```
Questo e' uno dei pochi casi in cui siamo costretti a inserire \_\_m128 come elemento di uno struct, causando loads aggiuntivi a memoria.
```
struct EventArgs_t
{
    union {
        EventComplexArg_t* c;
        Byte_t*            p;
        U64_t              u64;
        I64_t              i64;
        // ...
    } idata[2];
    V128f fdata[2];
};
struct Event_t
{
    Sid64_t type;
    EventArgs_t args;
};
```
E questo e' l'oggetto trasportato durante l'event handling.

## Event Handling
ogni entita', modulo, game object che sia, puo' diventare un event listener per un particolare tipo di evento. Un approccio object-oriented 
sarebbe quello di definire una interfaccia `IEventListener` con metodo `onEvent` che si traduce in uno `switch` statement in tutti i 
possibili eventi in ascolto (evitiamo la proliferazione di tante funzioni di event handling diverse per ogni tipo di evento, il che e' ottimo)

### Chain of Responsability
Nella gestione di eventi particolare attenzione e' posta alle interdipendenze tra i game objects

Nel modello che vogliamo implementare, le entita' sono composte da componenti e sono legate tra loro con legami di parentela, formando una 
struttura gerarchica, lo __scene graph__.

Gli eventi saranno passati direttamente agli oggetti padre, che a loro volta li passeranno
    * ai propri componenti
    * agli oggetti figli

*Quali oggetti padre?*
Non tutte le entita' sono interessate a ricevere un determinato evento. Dunque si deve fornire la possibilita' alle varie entita' di 
*registrarsi* ad un evento, e le entita' piu' in alto nella gerarchia registrate ad un dato evento saranno quelle che lo riceveranno.
* Una entita', che in se e' puramente un guscio di componenti 
  (vedi [ECS](https://www.simplilearn.com/entity-component-system-introductory-guide-article)),
  e' registrata ad un evento se almeno uno dei suoi componenti e' registrata ad un evento

Cio' vuol dire che entita' figlie/componenti non interessati ad un dato evento lo riceveranno in ogni caso per propagazione? L'alternative sarebbe
memorizzare la lista di componenti interessati, e inoltrare gli eventi solo a questi ultimi. Cio' comporta il dover gestire il caso in 
cui una entita' e' rimossa dalla scena.
```
// Pseudocodice, perche' bisogna integrare questo sistema con l'event handling di entt
virtual bool SomeComponent_t::onEvent(Event_t event)
{
    // handle the event (I am a component)
    switch (event.sid) 
    {
    case SID("EVENT_ATTACK"): 
        doSomething(attackInfoFromEventData(event));
        break;
        // other events ...
    }
}

virtual bool SomeEntity_t::onEvent(Event_t event)
{
    WeakRef<SceneNode> node = sceneGraphModule.graphAtRoot(this);
    Optional<ListView> optChildren = node.children();

    // handle the events yourself
    components.forEach([&event](Component_t* component){
        component.onEvent(event);
    });

    // then wake up child nodes
    optChildren.mbind([&event](ListView<Entity_t*> children)
    {
        children.forEach([&event](Entity_t* child){
            child.onEvent(event);
        });
    });
}
```

### Queued Event Handling
Al fine di poter mantenere il motore di gioco in uno stato consistente, e gestire gli eventi in modo deterministico, tutti gli eventi (incluso 
il framebuffer resize, anche se quello e' un caso a parte) non sono gestiti e propagati immediatamente, ma sono accodati in una queue memorizzato nello
slot 1 (stack di dati permanenti per il motore di gioco).

La difficolta' sta nella presenza di `EventComplexArg_t`s, i quali devnono essere duplicati mediante __deep copy__.
Il deep copying implica che ci debba essere dello spazio in memoria riservato a questi oggetti, i quali 
* sono di grandezza variabile, dunque non adatti al pool allocator
* potrebbero permanere in memoria per piu' di un frame, quindi non sono adatti al transient double buffer allorator degli slot 1,2
* hanno vita imprevedibile, dunque non adatti allo stack allocator dello slot 4, nel quale risorse piu' in basso nello stack potrebbero permeare in 
  memoria finche' gli eventi non sono stati risolti

rimane solamente lo slot 1, che rimane allocato per tutta la durata dell'applicazione.
La struttura dati che deve gestire questi `EventComplexArg_t`s deve necessariamente essere un compromesso tra un array e una linked list, nel quale 
ciascun elemento e' del tipo
    | Next | Lunghezza | Event sid | Argument index | data ... |

nota come la presenza di un "next pointer" e "lunghezza" e' ridondante ma necessaria per gestire il caso in cui e' si tratta dell'ultimo elemento di 
un blocco continuo
