# TODO:
## General
1. Crear clase GPIO que englobe todas las incializaciones necesarias y lleve la cuenta de los pines utilizados? Que sea punto intermedio para todos los drivers que utilicen GPIO (ejemplo SPI o I2C).
2. Inicialización de los clocks? Cambia según los modulos que tengo que inicializar?
## I2C
1. Investigar sobre las prioridades de interrupcion (I2C) y configuracion de prioridades NVIC para las interrupciones de I2C?
2. Testear todos los I2C
4. Destructor (desregistrar el drvier y demás)
5. DeInit de GPIO y NVIC
6. Ver que pasa con los Devices si hago deinit del bus (tendria que pasar el bus a nullptr en cada device asociado)
7. Hacer cada uno de los callbacks adecuados para I2C.
8. Que pasa si no puedo enviar transacción luego de terminar otra? Poner timer para volver a intentar?
9. Ver de hacer las funciones que reciben `I2cTransaction` reciban `const I2cTransaction`