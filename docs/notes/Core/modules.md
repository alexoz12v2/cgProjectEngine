## Modules System

Una applicazione che utilizza cgProjectEngine non definisce l'entrypoint (main). Bensi', essa e' definita dalla libreria.
In seguito, vengono caricati tutti i **Moduli** di cgProjectEngine, e a seguire tutti i moduli dichiarati dall'applicazione.
Una applicazione deve essere in grado di dichiarare piu' moduli, che corrispondono ad un target cmake.
Questi moduli possono definire 
* una scena
* funzionalita' condivise

Almeno uno dei moduli di tipo "scena" dichiarato da cgProjectGame deve essere marcato come **Startup Module**, 
affinche` cgGameEngine sappia quale modulo caricare per primo.
Soltanto un modulo di tipo "scena" puo' essere caricato in un qualsiasi momento

I moduli "shared" invece rappresentano funzionalita` condivise, e come tali, multipli moduli shared possono essere caricati.

Non possono esistere 2 istanze di moduli dello stesso tipo.

Possibile implementazione di uno Shared Module:

```
class HelloWorldModule : public SharedModule 
{
public:
    static Id moduleId = "HelloWorldModule"_sid; // necessary so that we can retrieve the module from other modules
    HelloWorldModule() : SharedModule() { m_id = moduleId }

    void onInit(SharedModuleParams params) override 
    {
        auto msg = (char const*)params.u64;
        m_msg = msg;
    }

    void onDestroy() override {}

public:
    void print() { fmt::print("{}", m_msg); }

private:
    char const* m_msg;
};
```

**Tutti Gli SharedModules sono caricati all'avvio, prima dello startup module**
Possibile implementazione di uno SceneModule, taggato come startup module

```
class MockGameStartModule : public SceneModule
{
public:
    static Id moduleId = "MockGameStartModule"_sid;
    MockGameSartModule() : SceneModule() 
    { 
        m_id = moduleId;
        yamlParser = getModule<YamlParserModule>(YamlParserModule::moduleId, {u64 = (uint64_t)"hello world"}); // if has't been created yet, pass params
        helloWorld = getModule<HelloWorldModule>(HelloWorldModuel::moduleId);
    }

    void onInit(SharedModuleParams params) override
    {
        yamlParser.parseAs<World>("assets/level1.yml"); // HOW TO INSTANTIATE?
    }

    void onTick(float deltaTime) 
    {
        helloWorld.print();
    }

    void onDestroy() override 
    {
    }

private:
    HelloWorldModule helloWorld;
    YamlParserModule yamlParser;
};

g_startupModuleId = MockGameStartModule::moduleId;

```

** Per non incorrere nel costo di virtual functions per ogni chiamata a funzione, potenzialmente eseguita ogni frame, i moduli interni non seguiranno questa struttura, bensi' una struttura custom a seconda dei requisiti di ciascun modulo**

# Gestione di dipendenze dei Moduli e Inizializzazione
Ciascun modulo aggiunto alla applicazione puo' richiedere i servizi di un altro
modulo, dunque deve essere inizializzato prima del richiedente.
Si va logicamente a generare un grafo aciclico direizionato di dipendenze di ciascun modulo.

# Sincronizzazione tra moduli
boh
