# ESP32C3 SUBWOOFER DSP V1.0

CHEAPEST SUBWOOFER DSP WITH ONLY ONE ESP32 C3 AND PCM5102 (INMP441 IN PROCESS)

-ESPHOME (HOME ASSISTANT) BASED DEVELOPMENT AND EXTERNAL COMPONENT (.H .CPP)

-ESP32C3 WITH OLED 0.42" DISPLAY.

-PCM5102 DAC CONVERTER (DIGITAL TO ANALOG AUDIO CONVERTER), AUDIO OUT TO AMPLIFIER.

-AUDIO INPUT, DIRECT TO GPIO4 ADC ESP32C3 PIN ;-) see SCHEMA-PINS BEFORE 
  
-INMP441 MEMS MICROPHONE - AUTO EQ, AUTO TUNE, DYNAMIC BOOST LOW FREQUENCIES, SUBWOOFER RESPONSE CURVE ANALYSIS. (IN PROCESS)

motoleonx@gmail.com (youtube https://www.youtube.com/@motoleonx)



# LFE DSP: Procesador Digital de Subwoofer de Código Abierto

## Control profesional para tu subwoofer, sin las limitaciones de una caja negra comercial

LFE DSP es un procesador de señal digital dedicado a subwoofers, construido sobre un ESP32-C3 y una cadena de audio completamente de **24 bits reales**, de principio a fin. Nace de un proyecto personal con un objetivo claro: llevar el control de graves que normalmente solo encuentras en procesadores profesionales de varios cientos de euros a un dispositivo abierto, transparente y totalmente ajustable — tanto en un home cinema como en un coche.

---

## Cadena de audio sin compromisos

- **Entrada y salida a 24 bits reales**, sin reducir la resolución en ningún punto del camino (ADC PCM1808 de entrada, DAC PCM5102 de salida).
- **Procesamiento en punto fijo de alta precisión** (Q24/Q8), diseñado para exprimir el rendimiento de un microcontrolador de bajo coste sin sacrificar la calidad de audio ni saturar la CPU.
- **Canal I2S único y persistente**: el reloj de audio nunca se interrumpe al cambiar de fuente, evitando los clics y la desincronización que sufren muchos diseños DIY equivalentes.

## Filtrado y corrección tonal

- **Filtro paso-alto (HPF)** ajustable de 10 a 30 Hz, con 1 o 2 etapas, para proteger el driver de infrasonidos sin robarle un ápice de grave útil.
- **Filtro paso-bajo (LPF)** configurable de 40 a 250 Hz, con 1 a 3 etapas en cascada. Con 2 etapas, el filtro se comporta como un **Linkwitz-Riley de 4º orden (LR4)** — el estándar de facto de la industria para una integración sin bultos ni huecos en el punto de corte.
- **Ecualizador paramétrico (PEQ)** de frecuencia, Q y ganancia totalmente ajustables, pensado para corregir con precisión quirúrgica los picos de resonancia de la sala o del habitáculo del coche — sin recurrir a un notch de corte total que destruya la respuesta natural del sistema.
- **Loudness dinámico** con techo de referencia propio: refuerza el grave de forma inteligente cuando el nivel de escucha es bajo, y se retira automáticamente cuando el volumen sube, evitando el exceso de graves a todo volumen.

## Medición y auto-calibración integradas

- **Barrido de frecuencia automático** de 20 a 120 Hz — el rango de trabajo estándar de un canal LFE — que mide la respuesta real de tu subwoofer en tu propia sala o coche, usando un micrófono de medición integrado (INMP441).
- **Detección automática de picos de resonancia (Auto-PEQ)**: tras el barrido, el sistema localiza el pico dominante, estima su ancho de banda real mediante el método profesional de "puntos de media ganancia", y aplica automáticamente la corrección de frecuencia, Q y ganancia — con salvaguardas para descartar ruido de medición y evitar sobrecorregir.
- **Generador de tono senoidal puro** integrado, para pruebas manuales y diagnóstico, con frecuencia y ganancia ajustables desde la propia interfaz.

## Protección del sistema

- **Limitador de picos** con techo configurable, para evitar el clipping en las crestas de señal más exigentes.
- **Limitador RMS/térmico independiente**, que protege contra el exceso de energía sostenida sin necesidad de recortar los picos puntuales — dos mecanismos de protección complementarios, no uno solo.
- **Modo noche**: un solo interruptor comprime más los picos, refuerza el grave relativo a bajo volumen y reduce la ganancia general, para disfrutar de tu sistema sin molestar, con restauración automática de tu configuración habitual al desactivarlo.

## Ajuste fino de la integración con tu sistema

- **Retardo (delay)** configurable de 0 a 20 ms y **control de fase** de 0 a 180°, para afinar la integración temporal entre el subwoofer y el resto de altavoces.
- Compatible con la práctica estándar de bass management de home cinema: crossover de referencia en 80 Hz (el valor recomendado por THX), con margen de medición hasta 120 Hz, el límite superior habitual del canal LFE en las normas de audio multicanal (EBU Tech 3276).

## Abierto, transparente y bajo tu control

- Firmware **100% ESPHome**, sin nube, sin suscripciones ni telemetría oculta — se integra directamente en tu instalación de Home Assistant.
- Cada parámetro es visible y ajustable en tiempo real, con sensores de diagnóstico (RMS, picos, temperatura del chip, reducción de ganancia de cada limitador) para entender exactamente qué está haciendo tu sistema en cada instante.
- Pensado desde el primer día para funcionar tanto en una **instalación de home cinema** fija como en un **sistema de audio de coche**, sin necesidad de hardware adicional para la medición y calibración.

---

*LFE DSP es un proyecto personal de electrónica de audio DIY. No es un producto certificado ni sustituye a un ingeniero de acústica profesional, pero incorpora los mismos principios y métodos que emplea la industria de audio profesional para el diseño de crossovers y la corrección de resonancias.*
