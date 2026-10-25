# Owly para KDE Plasma 6

Un elemento gráfico de panel para enviar y recibir archivos con AirDrop.
La vista compacta ocupa un icono y abre un desplegable. También se puede
colocar en el escritorio o habilitar en la configuración de la bandeja del sistema.

## Instalar

Requiere Plasma 6, Qt 6.6 o posterior con sus archivos de desarrollo (Core,
Gui, Qml, Quick y DBus), KDE Frameworks 6 KIO con sus archivos de desarrollo,
CMake, un compilador C++17 y Rust. Las miniaturas usan los complementos de KIO
(por ejemplo, `kio-extras` para imágenes y documentos, y `ffmpegthumbs` para vídeo).
Si no hay un complemento disponible, se muestra el icono del tipo de archivo.
En ejecución usa los módulos QML de Plasma Components, Plasma Extras, Kirigami y Qt Quick Dialogs,
normalmente incluidos con Plasma. No requiere Python para funcionar.

Desde la raíz del proyecto:

```sh
bash plasma/install.sh
```

Compila la CLI actualizada y el adaptador de presentación; instala la CLI en
`~/.local/bin/owly` y el plasmoide en el directorio de datos de tu usuario.
Después: **Editar panel → Añadir elementos gráficos → Owly KDE — AirDrop**.
Para actualizar, vuelve a ejecutar el mismo comando. Si el widget estaba abierto,
cierra sesión y vuelve a entrar: Plasma puede conservar la biblioteca nativa
anterior en memoria.

`owlyd` y su política de autorización se instalan mediante el
[procedimiento habitual de Owly](../README.md#instalación). El instalador del widget
no modifica servicios, reglas de cortafuegos, configuración de red ni el panel.
El widget muestra el diagnóstico existente si falta alguno de esos requisitos.

Para compilar solo el widget:

```sh
cmake -S plasma -B target/plasma -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build target/plasma --parallel 2
kpackagetool6 --type Plasma/Applet --install target/plasma/package
```

También se genera `target/plasma/io.github.eirom16.owlykde.plasmoid`, instalable con
**Añadir elementos gráficos → Instalar desde archivo local**. Incluye una
biblioteca nativa: compílalo en el sistema donde se utilizará. La CLI debe ser
la de este checkout, con soporte para los eventos `--json`.

## Uso

1. Activa una sesión. De forma predeterminada dura diez minutos y admite
   solicitudes de recepción. Si la tarjeta usa un modo exclusivo, se avisa
   antes de activar porque el Wi-Fi se interrumpe temporalmente.
2. Para enviar, selecciona o arrastra archivos al desplegable, elige un
   dispositivo y pulsa **Enviar**. **Buscar dispositivos** repite la búsqueda.
3. Para recibir, cada solicitud muestra quién envía y qué ofrece. Acepta o
   rechaza antes del plazo de 15 segundos que ya establece el receptor.
4. **Descargas** abre la carpeta de destino. **Actividad** muestra las últimas
   veinte transferencias o errores durante la vida de esa instancia del widget.
   Cada archivo enviado o recibido muestra su miniatura y nombre. Púlsalo para
   abrirlo con la aplicación del sistema; el botón de carpeta abre su ubicación.
   Los lotes muestran una fila por archivo. Si se mueve o elimina un archivo,
   al abrirlo se informa de que ya no está disponible en su ubicación original.
   Los detalles de los errores se pueden seleccionar y copiar.

Cerrar el desplegable mantiene la sesión activa. Desactivar, agotar el plazo
o retirar el widget libera la radio mediante la CLI y el servicio existentes.
Los enlaces recibidos se muestran como texto y no se abren automáticamente.

En **Configurar Owly** puedes cambiar el nombre anunciado, carpeta de destino,
duración de sesión y ruta del ejecutable. Los cambios se usan en la siguiente
sesión. Una ruta vacía detecta primero `~/.local/bin/owly` y después `PATH`.

## Integración visual

La interfaz usa `PlasmoidItem`, Plasma Components y Plasma Extras; el marco
del desplegable lo dibuja Plasma. Los controles, iconos, tipografía, foco,
colores semánticos y espaciados proceden del sistema mediante Kirigami.
No fija una paleta, fuente ni fondo propios. Las páginas de configuración
emplean `Kirigami.FormLayout` y controles de escritorio.

El [icono de AirDrop en SVG](package/contents/icons/owly-airdrop.svg) se muestra
como máscara del color de texto de Plasma. El mismo archivo sirve para temas
claros y oscuros; el instalador lo registra también en los iconos del usuario.

Referencias de KDE: [propiedades del widget](https://develop.kde.org/docs/plasma/widget/properties/)
y [API de Plasma 6](https://develop.kde.org/docs/plasma/widget/porting_kf6/).

## Arquitectura y comprobaciones

`src/backend.cpp` es un adaptador Qt de procesos y estado visual. Ejecuta
`owly doctor`, `radio up`, `browse`, `send` y `receive` con argumentos separados,
sin shell. La CLI añade eventos JSON por línea y decisiones de recepción
`{"id":7,"accept":true}` por stdin; el driver existente sigue gestionando
el protocolo, vencimiento de solicitudes, almacenamiento y transferencias.
Los comandos sin `--json` conservan su salida de terminal.

El widget usa una concesión de radio vinculada al proceso, nunca una concesión
separada. No adquiere privilegios propios. La recepción no usa `--accept`.
Los procesos trabajan de forma asíncrona y sus registros se limitan en memoria.

Para ejecutar pruebas del adaptador, se necesitan Qt Test, KF6I18n, los
complementos de miniaturas de imágenes (`kio-extras`) y Python 3:

```sh
cmake -S plasma -B target/plasma -DBUILD_TESTING=ON
cmake --build target/plasma --parallel 2
ctest --test-dir target/plasma --output-on-failure
cargo test --locked -p owly-cli
```

La prueba de procesos utiliza una CLI simulada que no toca la red ni la radio.
Comprueba decisiones, argumentos con caracteres especiales, eventos fragmentados,
cancelación, archivos inválidos, apertura explícita de archivos del historial,
archivos eliminados y liberación de la sesión. Otra prueba genera una miniatura
PNG real mediante KIO y verifica su proporción y el cambio a un archivo ausente.
Para una vista previa de la interfaz real con dispositivos simulados:

```sh
target/plasma/owly-preview --fixture
target/plasma/owly-preview --fixture --dark
target/plasma/owly-preview --fixture --small
```

Si está instalado el SDK de desarrollo de Plasma y Extra CMake Modules, puedes
añadir `-DOWLY_TEST_PLASMA=ON` a CMake. Se habilita una segunda prueba que carga
el paquete completo mediante libplasma, inicia una sesión simulada y verifica
que aparecen los dispositivos. Admite capturas con `plasmoid-test --capture
/tmp/owly.png`, además de `--dark` y `--small`.

Sin `--fixture`, la vista previa usa la CLI instalada. La comprobación final de
AirDrop requiere un dispositivo Apple real y una tarjeta compatible.
