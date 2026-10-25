# Cambios de Owly KDE

La versión del widget KDE es independiente de la versión del motor de Owly.

## 0.1.2 — En desarrollo

- Nuevo icono SVG inspirado en AirDrop, con cuatro arcos abiertos y trazos redondeados.
- El icono del panel y el encabezado siguen el color de texto del tema de Plasma,
  incluidos los modos claro y oscuro.
- El instalador registra también el icono en el tema de iconos del usuario.

## [0.1.1 — primera versión preliminar](https://github.com/Eirom16/owly-kde/releases/tag/kde-v0.1.1)

Motor incluido en el código fuente: Owly 0.4.1.

### Interfaz para Plasma

- Elemento gráfico de panel, bandeja del sistema y escritorio para Plasma 6.
- Controles, colores, iconos, tipografía y espaciado integrados con el tema del sistema.
- Búsqueda de dispositivos, selección y arrastre de archivos, envío y cancelación.
- Recepción con aceptación explícita, notificaciones e historial de actividad.
- Nombre visible, carpeta de destino y duración de sesión configurables.
- Diagnóstico de la radio y aviso cuando el modo exclusivo interrumpe el Wi-Fi.

### Correcciones incluidas

- El envío vuelve a localizar el dispositivo por nombre mediante la CLI, en
  lugar de fijar una dirección AWDL que puede haber cambiado desde la búsqueda.
- La interfaz distingue la búsqueda del dispositivo y la preparación del envío.
- Los errores quedan en Actividad y su detalle se puede seleccionar y copiar.

### Distribución y validación

- Instalador para el usuario y paquete `.plasmoid` con adaptador Qt nativo.
- Pruebas del adaptador, pruebas opcionales de carga mediante libplasma y CI para Plasma.
- Envío y recepción comprobados manualmente con un iPhone en el equipo del mantenedor.
- Publicación preliminar; la compatibilidad con otros equipos depende del hardware
  y de los requisitos descritos en el README y en las notas de la release.

El trabajo previo a esta publicación no tuvo una release pública KDE 0.1.0.
Los cambios anteriores del motor se consultan en [Owly](https://github.com/t4t5/owly).
