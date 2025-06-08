# Tiny Monte Carlo

- [Página en Wikipedia sobre el problema](https://en.wikipedia.org/wiki/Monte_Carlo_method_for_photon_transport)
- [Código original](https://omlc.org/software/mc/) de [Scott Prahl](https://omlc.org/~prahl/)

# Vectorizacion
Comandos para compilar el codigo assembler:

```bash
gcc -std=c11 -Wall -Wextra -O3 -ftree-vectorize -fopt-info -fopt-info-missed -funsafe-math-optimizations -ffast-math -march=native -fverbose-asm -S photon.c -o photon.s
```

```bash
gcc -std=c11 -Wall -Wextra -O3 -ftree-vectorize -fopt-info -fopt-info-missed -funsafe-math-optimizations -ffast-math -march=native -fverbose-asm -S tiny_mc.c -o tiny_mc.s
```

Las flags `-fopt-info` y `-fopt-info-missed` muestran por consola las optimizaciones realizadas y las optimizaciones que no logró realizar, respectivamente. También la flag `-fverbose-asm` escribe comentarios en el assembler generado.  
Las demás flags agregadas son con intención de ayudar a autovectorizar al compilador.

Revisando el assmebler generado podemos observar que el compilador no es capaz de autovectorizar debido a que utiliza instrucciones de scalar simple.

