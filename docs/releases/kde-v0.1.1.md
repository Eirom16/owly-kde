# Owly KDE 0.1.1 — Primera versión preliminar

AirDrop desde el panel de KDE Plasma: envía y recibe archivos con dispositivos
Apple mediante una interfaz que sigue el tema de tu escritorio.

Esta es la primera release pública de la interfaz KDE. El widget tiene versión
**0.1.1** y utiliza el motor **Owly 0.4.1**. Se publica como **prerelease** para
ampliar las pruebas a más tarjetas y equipos.

## Incluye

- Icono de panel y desplegable con integración visual en Plasma 6.
- Búsqueda de dispositivos, selección y arrastre de archivos, progreso y cancelación.
- Recepción con aceptación o rechazo explícitos y notificaciones del escritorio.
- Carpeta de destino, nombre visible y duración de sesión configurables.
- Actividad con transferencias y errores cuyo detalle se puede copiar.
- Corrección del envío: se vuelve a localizar el teléfono al enviar en lugar de
  fijar una dirección AWDL guardada durante la búsqueda.

[Capturas e instrucciones completas](https://github.com/Eirom16/owly-kde/blob/kde-v0.1.1/README.md).

## Elegir la descarga

| Archivo | Uso |
| --- | --- |
| `owly-kde-0.1.1.tar.gz` | Código fuente completo del fork; opción recomendada para compilar en tu distribución. |
| `owly-kde-0.1.1-arch-cachyos-x86_64-qt6.12.plasmoid` | Widget precompilado en CachyOS x86_64 con Qt 6.12; requiere un entorno compatible. |
| `BUILD-INFO.txt` | Revisión del código, entorno de compilación y dependencias del binario. |
| `SHA256SUMS` | Sumas de comprobación de los archivos adjuntos. |

El `.plasmoid` contiene una biblioteca nativa. La instalación de la CLI con
soporte `--json` y del servicio `owlyd` se realiza por separado. Para otros
entornos, compila desde el código fuente siguiendo el README.

## Instalación desde el código fuente

```sh
tar -xzf owly-kde-0.1.1.tar.gz
cd owly-kde-0.1.1
```

Instala primero el servicio con los pasos del
[README de esta versión](https://github.com/Eirom16/owly-kde/blob/kde-v0.1.1/README.md#instalación).
Después instala el widget y la CLI para tu usuario:

```sh
bash plasma/install.sh
```

Añádelo desde **Editar panel → Añadir elementos gráficos → Owly KDE — AirDrop**.
Si actualizas una instalación que estaba abierta, cierra sesión y vuelve a
entrar para que Plasma cargue la biblioteca nativa nueva.

## Validación y alcance

- Envío y recepción comprobados con un iPhone y una tarjeta Intel `iwlwifi`
  (`8086:2526`) en el equipo del mantenedor.
- Pruebas del motor y del adaptador Qt, carga del plasmoide y temas claro y oscuro.
- Las comprobaciones del motor y del widget en GitHub Actions pasan para el
  código funcional de esta versión.
- La compatibilidad de radio sigue dependiendo de la tarjeta. El modo exclusivo
  interrumpe temporalmente el Wi-Fi; el widget avisa antes de activarlo.
- Para recibir en el dispositivo Apple, habilita **Todos durante 10 minutos**
  y mantén la pantalla desbloqueada. No se implementa el modo Solo contactos.

Si encuentras un fallo, copia el detalle desde **Actividad** e indica la
distribución, versiones de Plasma y Qt, tarjeta Wi-Fi y versión de iOS.

Basado en [t4t5/owly](https://github.com/t4t5/owly). Licencia MIT.
