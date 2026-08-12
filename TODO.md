# TODO

## General
1. Crear clase GPIO que englobe todas las incializaciones necesarias y lleve la cuenta de los pines utilizados? Que sea punto intermedio para todos los drivers que utilicen GPIO (ejemplo SPI o I2C).
2. Inicialización de los clocks? Cambia según los modulos que tengo que inicializar?
3. **Pasar todo el proyecto por las convenciones de `CLAUDE.md`.** Se aplicaron a medida que
   se tocaba cada archivo, asi que el codigo mas viejo (`i2c`, `timer`, `lib`,
   `authentication`, `controller`, `persistence`) todavia no las cumple. Concretamente:
   - **Indentacion**: un espacio entre tipo y nombre, sin alinear en columnas. Detectar con
     una busqueda de dos o mas espacios seguidos fuera de la indentacion y de los comentarios.
   - **Banners**: sacar las reglas `// =====` de los comentarios de clase (rompen el render
     del hover, que interpreta la linea de `=` como encabezado Markdown). Quedan ~22 archivos.
   - **Llaves**: expandir los `if(x) { a; b; }` de mas de una sentencia.
   - **Orden de miembros**: tipos, despues atributos, despues metodos.

## Controller
1. **Ancho de direccion configurable** (registros y opcodes). Hoy ambos son de 1 byte:
   128 registros y 256 opcodes, con `0x6C-0x7D` libres y rangos de comando de sobra, asi
   que nada lo empuja todavia. Cuando haga falta, tener en cuenta que no es contenible
   como lo fue el tamanio del banco:
   - `ControllerSlave` toma el **primer byte escrito** como puntero de registro, dentro
     del ISR. Con 2 bytes necesita acumular antes de saber a que registro apunta.
   - El tipo se propaga a `RegisterBank`, `RegisterWindow`, los offsets de cada seccion
     y el `Router`.
   - Es una **decision de protocolo**: hay que acordarla con el master, no alcanza con
     generalizar este lado. El driver de I2C ya soporta registros multi-byte
     (`withRegister(reg, length)`); el que asume 1 byte es `ControllerSlave`.
2. `RegisterBank::read()` toma `uint16_t` y `setRegister()` toma `uint8_t`. Unificar.
3. Los registros del mailbox (`REG_REQUEST`/`REG_RESPONSE`) no se reservan en el banco, asi
   que una ventana mal dimensionada podria pisarlos. Lo natural seria que `ControllerSlave`
   los reclame al construirse, pero depende del orden de construccion contra las ventanas.
4. **Unificar como se ven las lecturas de ventana con como se ven los comandos.** Hoy son
   dos modelos separados (registros inmediatos vs mailbox diferido, protocol-design.md §6).
   La idea a evaluar: que leer una ventana se consuma con la misma forma que un comando,
   con la unica diferencia (enmascarada adentro de la clase) de que el registro es fijo y
   la respuesta es inmediata — el consumidor solo sabe que le llego el dato del "comando".

## Authentication
1. **Las sesiones pueden sobrevivir a su vigencia cuando el reloj da la vuelta.**
   `Session::start()` guarda `expireTime = currentTime + validitySeconds` y
   `Session::isExpired()` compara en absoluto (`currentTime >= expireTime`). La fuente es
   `HAL_GetTick()/1000`, y `HAL_GetTick()` es `uint32_t` en ms: **vuelve a cero cada ~49,7
   dias**, dividir por 1000 cambia la granularidad pero no el periodo de vuelta. Entonces:
   - una sesion abierta poco antes de la vuelta obtiene un `expireTime` que envuelve y
     **expira al instante**;
   - una abierta bastante antes queda con `currentTime` chico despues de la vuelta y **no
     expira hasta que el contador la alcance**, semanas mas tarde.

   Lo segundo es lo grave: un token valido mucho mas alla de su vigencia, en el sistema que
   autoriza cambios de calibracion. El arreglo es aritmetica por diferencia, correcta ante
   wrap con unsigned: guardar `startTime` + `validity` y comparar
   `(currentTime - startTime) >= validity`. Ojo con el uso actual de `expireTime == 0` como
   marca de slot limpio — `!user || !token` ya lo cubre.

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

## Builders
1. **Que el `Config` solo lo pueda fabricar su `Builder`.** Hoy `I2cBus::Config` y
   `Timer::Config` son construibles por cualquiera, y los campos requeridos de
   `I2cBus::Config` (`bus`, `clockSpeed`) **no tienen inicializador**: un `Config` armado
   a mano y no completado no da un valor invalido, da basura sin inicializar. La salida es
   la que ya se aplico en `PeerMonitor::Config`: constructor por defecto privado +
   `friend class Builder`, asi la unica fuente de un `Config` es el builder. Verificable:
   `I2cBus::Config c;` tiene que dejar de compilar.

2. **Como reporta el builder una config incompleta.** `PeerMonitor::Builder::buildConfig()`
   devuelve `lib::Result` y rechaza con `NoPeer`. Los de `i2c`/`timer` no pueden adoptar eso
   sin coordinarse con `docs/exceptions-removal.md`, que ya decidio que esas mismas
   violaciones de precondicion pasan a `assert()`, no a `Result`. Hacer las dos cosas por
   separado deja el peor estado intermedio: el builder devolviendo `Result` mientras
   `I2cBusStatic::init()` sigue tirando `std::logic_error`.

   Al decidirlo, revisar tambien `PeerMonitor` por coherencia: con el constructor privado
   ya no se puede fabricar un `Config` sin peer, asi que su `Result` podria ser un `assert`
   como el resto — o el resto podria ser `Result` como el. Lo que no conviene es que
   convivan los dos criterios sin una razon.

   Son 33 call sites de builders, sobre codigo validado en hardware: conviene hacerlo junto
   con la migracion de excepciones, que toca esos mismos archivos.

3. **Revisar donde mas convendria un builder y todavia no lo hay.** Hoy lo usan `I2cBus`,
   `Timer`, `I2cTransaction` y `PeerMonitor`. Candidatos a mirar: `Ads1115` (hoy se
   configura con setters sueltos post-construccion — ver "ADS1115" mas abajo, que ya
   discute exponer `Config`), `FlashStore`, `StaticUserManager`, `ControllerSlave`.
   El criterio para aplicarlo, no aplicarlo por defecto: cuantos parametros tiene el ctor,
   cuantos son opcionales o tienen default sensato, y si hay combinaciones invalidas que
   convenga rechazar en un solo lugar. Con dos o tres parametros obligatorios y ninguna
   combinacion invalida, un ctor directo es mas claro que un builder.

## Excepciones
Ver `docs/exceptions-removal.md` para el plan completo. Resumen: `-fno-exceptions` ya es el
default del proyecto; quedan 3 componentes que lo reactivan (`authentication` — vestigial,
sin ningun `throw`; `drivers/i2c`; `drivers/timer`).
