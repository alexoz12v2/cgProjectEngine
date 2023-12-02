# Math

Classi che devono essere supportate: `Vector` e `Matrix`
per le quali si fa una __unica__ eccezione e andare a definire uno `struct` contenente tipi vettoriali.

Conseguenza: operazioni intense direttamente sullo stuct comporterebbero la generazione di troppe istruzioni 
`movps`/`vmovps` (VEX) per fare il load/store di array di floats/ints e quant'altro nei registri `xmm`.

Per ovviare cio', oltre alle solite operazioni di addizione, prodotto scalare, prodotto vettoriale, modulo, 
bisogna fornire anche la possibilita' di estrarre i dati grezzi dall'oggetto per poterli elaborare direttamente con 
delle istruzioni native del processore x86\_64, sfruttando il fatto che esso e' l'unica piattaforma supportata

Inoltre, la classe vettore supporta, fino a quattro componenti, il cosiddetto `swizzling`

# Time
