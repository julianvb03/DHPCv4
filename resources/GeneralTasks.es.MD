# Tareas para el Proyecto de Cliente y Servidor DHCP en C

## Profundizar dominio del problema DHCP
- [ ] **Modelado en GNS3:**
    - Crear dos simulaciones en GNS3 de como sería una arquitectura de red en la que se use un servidor DCHP de forma remota y local.
    - Aprender a usar un router como un **agente** o **DHCP Relay** que se encargue de transmitir las solicitudes de los clientes DHCP al servidor.

- [ ] **Dominio de Wireshark:**
    - Buscar como funciona wireshark y como podemos ver tanto la carga de los paquetes que se transmiten a travez de redes, como identificar cuando se usa un protocolo u otro mediante esta herramienta.
    - Usar Wireshark para ver los mensajes transmitidos en la simulación de la arquitectura.

- [ ] **Conocimientos en programación:**
    - Crear una conexión de sockets tipo UPC que sea capas de escuchar solicitudes broadcast, unicast y lanzar mensajes tipo bradcast y unicast, como forma de prueba para la implementación del servidor.
    - Investigar sobre como funciona la creaión de aplicaciones concurrentes en C **(vease PTHREADS)** y como se puede usan en conjunto con programación de sockets.
    - Aprender como hacer el parseo de un mensaje enviado por un socket usando structs en C.
    - Aprender que hace cada uno de los parametros que un servidor DHCP envia al cliente.
---

## Cliente DHCP

- [ ] **Inicialización de la comunicación DHCP:**
  - Crear una función que inicialice el cliente y configure el socket UDP para enviar mensajes de broadcast en el puerto 68.
  - Implementar una estructura que almacene la configuración de red (IP, máscara de subred, puerta de enlace, etc.).
  - [ ] **Implementación de mensajes DHCP:**
  - Implementar la construcción del mensaje **DHCPDISCOVER**.
  - Manejar la recepción de **DHCPOFFER** desde el servidor y parsear la oferta de red.
  - Implementar la respuesta **DHCPREQUEST** para solicitar el arrendamiento de la IP ofrecida.
  - Manejar la recepción del **DHCPACK** para confirmar la asignación de la IP.
  - Implementar el manejo de errores como **DHCPNAK (opcional)**  y timeouts.

- [ ] **Renovación y liberación de la IP:**
  - Implementar la lógica para enviar **DHCPREQUEST** en el proceso de renovación de la IP antes de que expire el arrendamiento.
  - Crear la función para liberar la IP enviando un mensaje **DHCPRELEASE (opcional)** cuando el cliente termina su uso.

- [ ] **Mostrar la configuración de red:**
  - Mostrar la IP asignada, máscara de subred, puerta de enlace, y otros parámetros una vez recibidos.
  - Implementar una función que muestre los tiempos de expiración y estado actual del arrendamiento.

- [ ] **Manejo de errores:**
  - Implementar manejo de errores cuando no se recibe una respuesta a tiempo, intentando retransmitir mensajes de forma inteligente.

---

## Servidor DHCP

- [ ] **Inicialización del servidor:**
  - Crear la función que inicializa el servidor, configura el socket UDP en el puerto 67 y escucha mensajes **DHCPDISCOVER**.
  - Configurar el pool de direcciones IP, máscara de subred, puerta de enlace y tiempos de arrendamiento.

- [ ] **Procesamiento de mensajes DHCP:**
  - Implementar el manejo de **DHCPDISCOVER** para asignar dinámicamente una IP del pool y responder con un mensaje **DHCPOFFER**.
  - Implementar la lógica para recibir **DHCPREQUEST** del cliente y enviar un **DHCPACK** confirmando la asignación de la IP.
  - Manejar mensajes **DHCPRELEASE** para devolver IPs al pool y permitir reasignaciones.
  - **(Opcional)** Implementar la lógica para manejar **DHCPDECLINE** y **DHCPNAK** en caso de errores.

- [ ] **Gestión de arrendamientos:**
  - Crear una estructura para manejar el tiempo de arrendamiento de las IPs, permitiendo renovaciones y liberaciones.
  - Implementar la función para rastrear qué direcciones IP están en uso y cuáles están disponibles.

- [ ] **Concurrencia:**
  - Añadir soporte para manejar múltiples solicitudes DHCP al mismo tiempo utilizando hilos o procesos en C (pthread o fork).

- [ ] **Registro de eventos:**
  - Implementar una función de registro que almacene cada evento de asignación, renovación o liberación en un archivo de log para llevar la trasabilidad de los usuarios.

---

## Tareas adicionales

- [ ] **Documentación:**
  - Crear la documentación detallada sobre la implementación del cliente y servidor DHCP, describiendo los mensajes, el flujo y la estructura del código.

- [ ] **Pruebas y validación:**
  - Probar el cliente y servidor en una red simulada o real para validar la correcta asignación y renovación de direcciones IP.
  - Implementar pruebas unitarias para cada función clave.

- [ ] **Seguridad básica (opcional):**
  - Implementar validaciones para evitar que IPs no válidas se asignen o que haya ataques como **DHCP spoofing** (opcional).
