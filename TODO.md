# TODO

## General
1. Crear clase GPIO que englobe todas las incializaciones necesarias y lleve la cuenta de los pines utilizados? Que sea punto intermedio para todos los drivers que utilicen GPIO (ejemplo SPI o I2C).
2. Inicialización de los clocks? Cambia según los modulos que tengo que inicializar?

## I2C
1. Investigar sobre las prioridades de interrupcion (I2C) y configuracion de prioridades NVIC para las interrupciones de I2C?
2. Testear todos los I2C
3. **Lectura de master de 1 byte (N=1) cuelga**: `prepareMasterRx(1)` asierta NACK+STOP
   antes de que el llamador limpie `ADDR`, y el I2Cv1 exige `NACK -> clear ADDR -> STOP`,
   asi que `RXNE` nunca se levanta. Afecta solo al STM32 como master. N=2 fue verificado
   y funciona.
4. ~~Destructor (desregistrar el drvier y demás)~~
5. ~~DeInit de GPIO y NVIC~~
6. ~~Ver que pasa con los Devices si hago deinit del bus (tendria que pasar el bus a nullptr en cada device asociado)~~
7. ~~Hacer cada uno de los callbacks adecuados para I2C.~~
8. ~~Que pasa si no puedo enviar transacción luego de terminar otra? Poner timer para volver a intentar?~~
9. ~~Ver de hacer las funciones que reciben `I2cTransaction` reciban `const I2cTransaction`~~
10. ~~Marcar que una transaccion fue erronea si entro en el handler de error~~
11. ~~No iniciar transaccion si el bus esta ocupado e iniciar un timer para volver a intentarlo. Sólo en los casos que el START falle o el bus este ocupado. No aplica para cuando se entra al handler de error~~
12. ~~Considerar cambiar `I2cBus` para que usen referencias a `I2cTransaction` en vez de copiar el objeto entero en la cola de I2cBus. El usuario ya es responsable de mantener el buffer de datos en memoria, encargarse de mantener el objeto en si mismo no es un salto muy grande y simplifica la gestion, además de permitir tener `I2cTransaction` fuertemente tipadas con headers para los callback (se puede definir una clase abstracta base y luego usar clases con templates).~~

## Interfaz Adc
Hoy `Adc` es la **vista del lector**: expone lecturas y rango, nada mas. Quien posee el
dispositivo usa el tipo concreto para configurarlo y dispararlo. Se dejo asi a proposito
(hay una sola implementacion y ningun consumidor real todavia), a revisar cuando se
implemente la adquisicion del reactor:

1. Configuracion generica (ganancia/rango, tasa de muestreo): hoy solo en el concreto.
   Los valores son especificos del chip, asi que la forma generica seria algo tipo
   `setFullScaleMicroVolts()` / `setSampleRateHz()` y que el device elija el mas cercano.
   Definir con un segundo ADC a la vista, no antes.
2. Disparo de lecturas (`startSample`): hoy solo en el concreto. Si corresponde subirlo a
   la interfaz depende de si el consumidor maneja la adquisicion o solo lee valores.

## ADS1115
1. **Configurar el ADC desde afuera de la clase.** Hoy `Config` es `protected` y la
   configuracion se expone como setters sueltos (`setGain`, `setDataRate`). Una alternativa
   es dejar que el llamador arme un `Config` completo y lo aplique de una
   (`ads.apply(Config()...)`). Para eso `Config` tiene que volver a ser publica, y
   `getBytes()` esconderse con `friend class Ads1115;` — la clase contenedora NO tiene
   acceso automatico a los privados de la anidada, hay que declararlo.

   **Sutileza a resolver antes:** hoy la config se aplica de forma *perezosa*. `setGain()`
   no escribe nada al chip; solo cambia el miembro, y se materializa recien en la palabra
   que arma `singleShotConfig()` en la proxima conversion. Eso vale porque somos
   single-shot y reescribimos la config en cada disparo. Un `apply()` externo tendria que
   definir si escribe al dispositivo en el momento o no.

2. **Modo continuo**: hoy el driver solo hace single-shot. Cambia la semantica del punto (1):
   en continuo la config se escribe una vez y persiste, no por conversion. Conviene
   diseniar ambas cosas juntas.

3. **Pin ALERT/RDY**: permitiria adquisicion por interrupcion en vez de polear el bit OS
   (que cuesta una transaccion I2C extra por conversion). Se configura con el comparador,
   asi que tambien pasa por `Config`.

## Excepciones
Ver `docs/exceptions-removal.md` para el plan completo. Resumen: `-fno-exceptions` ya es el
default del proyecto; quedan 3 componentes que lo reactivan (`authentication` — vestigial,
sin ningun `throw`; `drivers/i2c`; `drivers/timer`).
