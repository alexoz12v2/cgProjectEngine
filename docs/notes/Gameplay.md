# Gameplay system 

## Features richieste
* __runtime engine model__, object-centric, mediante l'utilizzo di un Entity-component-sytem tramite astrazione della libreria entt
* Serializzazione di un livello in Chunks

## Formato livello di gioco

In un livello di gioco, oggetto principe sono sicuramente le *Geometrie*, che possono essere elementi *Statici* o *Dinamici*.
Inoltre, ogni mesh puo' essere __istanziata__ molteplici volte, quindi adottiamo un approccio simile al formato obj, ovvero memorizzare una lista di 
tutte le mesh e scrivere nella struttura dati space-partioning un indice. Oltre a questo indice, 
per le mesh statiche sono necessarie anche informazioni sulle collisioni, dunque bounding boxes, o come capsule, o AABB, OOBB, k-DOP.
per le mesh dinamiche serve un formato serializzato di tutti gli attributi e componenti che rappresentano l'entita di gioco, con la loro configurazione
  iniziale (gli spawner)

tutte le porzioni di livello indicizzanti possono essere deserializzate nel pool allocator, mentre informazioni come triangle meshes, textures,
... sono serializzate nello stack allocator con una reference (Vedi Reference system)

## Reference

Il modo piu semplice per implementare un Reference system e' una _Handle Table_, indicizzata con degli stringids, la quale mappa tali identificatori
ad un puntatore contenente l'oggetto D. 
Per affrontare il problema dell'invalidazione delle references l'approccio seguito e' quello di un atomic reference counter, che, una volta sceso a 
zero, triggera la deallocazione dell'oggetto dal pool allocator

Le funzionalita necessarie:

* find object by id
* iterate over all objects satisfying a criterium (con interfaccia sia Range che Iterator)
* ray casting nella scena, e ritorna una lista di intersezioni o la prima intersezione
* find all objects in a given region (AABB)
