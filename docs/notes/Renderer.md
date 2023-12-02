# Renderer
Il sistema di rendering e' suddiviso in due porzioni distinte, racchiuse per motivi di compattezza in un unico modulo, seguono un approccio 
layerizzato
* `RHI`: renderizzatore di basso livello, che ragiona in termini di array di vertici e indici, quindi meshes, con handles alle texture associate.
   Qui si configura anche il materiale da utilizzare, in particolar modo i parametri del materiale, in quanto implementeremo soltanto un modello di 
   superficie (vedi Modello di Superficie)
* `Renderer`: renderizzatore di alto livello, sfruttera' le funzioni fornite da RHI per effettuare le tecniche di sotto enumerate

## RHI
Il Render Hardware Interface e' un componente che non astrae OpenGL in a granularita' 1:1. Non porterebbe alcun giovamento in quanto OpenGL e' l'unica
API grafica che utilizziamo. 
Piuttosto creiamo funzioni che settano lo stato per una particolare operazione grafica tra quelle che deve supportare il `Renderer`.
L'input che RHI accetta sara' espresso in basso livello, in termini di vertici, attributi, textures e altre primitive OpenGL. Il Renderer, che riceve 
in input la scena, rappresentata con una struttura gerarchica, sara' responsabile di trasformare e filtrare tale struttura per ricavare il formato 
di input che RHI richiede. 
Cio' permette, ad esempio, di concentrarci in termini di "Array Buffers" e non di "Bounding Volume Hierarchy". Ad esempio, le relazioni gerarchiche e 
i cambi di riferimento.



## High Level Renderer
### Modello di Superficie

Applichiamo la [Microfacet Theory](https://hal.science/tel-01291974/file/TH2015DupuyJonathan2.pdf), implementando la seguente equazione come macro BRDF
$$
   k_s f_{\text{\italic{spec}}}(\hat{n}, \hat{\omega_o}, \hat{\omega_l})
   + k_d f_{\text{\italic{diff}}}(\hat{n}, \hat{\omega_o}, \hat{\omega_l}), \\
   k_s + k_d = 1
$$
dove $f_spec$ modello di Cook-Torrance, utilizzante come BVF (Bi-Visibility Function) la Smith $G_2$ height-direction correlated, 
come NDF (Normal Distribution Function) La Throwbridge-Reitz, anisotropic version,
e dove F e' la l'equazione di Fresnel per la quantita' di Potenza Riflessa, approssimazione di Schlick se il materiale e' un dielettrico, 
formulazione con indice di rifrazione complesso altrimenti

$$
f_{\text{\italic{diff}}}(\hat{n},\hat{\omega_o},\hat{\omega_i}) 
  = \frac{F(\hat{\omega_h}, \hat{\omega_l})G_2(\hat{\omega_l}, \hat{\omega_o}, \hat{\omega_h})D(\omega_h)}
                                                {4\lvert \langle\hat{n},\hat{\omega_l}\rangle\rvert\lvert \langle\hat{n}, \hat{\omega_v}\rangle\rvert}
$$

e $f_{\text{\italic{diff}}}$ modello di Oren-Nayar, approssimato se troppo costoso,  con intregale approssimato allo scattering di secondo ordine, 
con $\rho$ surface albedo, $\sigma$ roughness. Essa definisce coefficienti preliminari
$$
c_1 = \frac{0.5\sigma^2}{\sigma^2+0.33}
c_2 = \frac{0.45\sigma^2}{\sigma^2+0.09}
c_3 = \frac{0.125\sigma^2}{\sigma^2+0.09}
c_4 = \frac{0.17\sigma^2}{\sigma^2+0.13}

\alpha = \max\left{\theta_o, \theta_i\right} = \max{\arccos(\langle \hat{n},\hat{\omega}_o\rangle), \arccos(\langle \hat{n},\hat{\omega}_i\rangle)} 

\beta = \min\left{\theta_o, \theta_i\right} = \min{\arccos(\langle \hat{n},\hat{\omega}_o\rangle), \arccos(\langle \hat{n},\hat{\omega}_i\rangle)}

\gamma = \cos(\phi_o - \phi_i) = 
  \frac{\langle\hat{\omega_o}\hat{\omega_i}\rangle - \langle\hat{n}\hat{\omega_o}\rangle\langle\hat{n}\hat{\omega_i}\rangle}
       {\sqrt{1-\langle\hat{h},\hat{\omega}_o\rangle^2}\sqrt{1-\langle\hat{h},\hat{\omega}_i\rangle^2}}

{c\prime}_2 = \gamma\begin{cases}\sin\alpha\;\gamma\geq 0\\{c\prime\prime}_2\end{cases}

{c\prime\prime}_2 = \sin\alpha - \left(\frac{2\alpha\beta}{\pi}\right)^3

{c\prime}_3 = (1-|\gamma|)\left(\frac{4\alpha\beta}{\pi^2}\right)^2
$$

da cui le equazioni per i due termini
$$
f_r^1(\hat{n},\hat{\omega_o},\hat{\omega_i}) = \frac{1}{\pi}\left(1-c_1+c_2{c\prime}_2\tan\beta 
                                                                  + c_3{c\prime}_3\tan\left(\frac{\alpha+\beta}{2}\right)
                                                            \right)
f_r^2(\hat{n},\hat{\omega_o},\hat{\omega_i}) = \frac{1}{\pi}c_4\left(1-\gamma\left(\frac{2\beta}{\pi}\right)^2\right)

f_{\text{\italic{diff}}}(\hat{n},\hat{\omega_o},\hat{\omega_i}) = \rho f_r^1(\hat{n},\hat{\omega_o},\hat{\omega_i})
                                                                + \rho^2 f_r^2(\hat{n},\hat{\omega_o},\hat{\omega_i})
$$
### Shadow Mapping
### Sorgenti Luminose Supportate
## Valutazione dei modelli di superficie: Illuminazione Globale
## Valutazione della illuminazione indiretta: Probe-based Lighting
### Il principio e Image Based Lighting
### Diffuse: Spherical Harmonics
### Specular: 
