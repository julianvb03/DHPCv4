# Documento de Dominio del Problema

## 1. Objetivo

El objetivo principal de este proyecto es desarrollar un **servidor DHCP** funcional y un **cliente DHCP**. El servidor debe ser capaz de manejar solicitudes de cualquier cliente conectado a la red, asignando dinámicamente direcciones IP y los parámetros de red necesarios. El cliente debe ser capaz de solicitar una dirección IP al servidor y comunicarse con este para obtener, renovar o liberar configuraciones de red según sea necesario.

## 2. Introducción

### Responsabilidades del Servidor:
- El servidor DHCP es responsable de escuchar las solicitudes de los clientes y asignar dinámicamente configuraciones de red, como direcciones IP, máscaras de subred y puertas de enlace predeterminadas.
- Gestiona un conjunto de direcciones IP y maneja el **arrendamiento** (duración) de las asignaciones de IP, permitiendo a los clientes renovar o liberar sus direcciones cuando sea necesario.
- El servidor debe asegurarse de manejar solicitudes concurrentes de múltiples clientes.

### Responsabilidades del Cliente:
- El cliente DHCP inicia la comunicación solicitando detalles de configuración de red, como una dirección IP, al servidor.
- El cliente debe ser capaz de interactuar con el servidor para recibir una dirección IP y gestionar las renovaciones o liberaciones de IP según sea necesario.
- Debe manejar el envío y la recepción de mensajes **DHCP** como **DISCOVER, REQUEST** y **RELEASE**.

### Configuración del agente en caso de conexión remota:
En el caso de una **conexión remota**, tanto el servidor como el cliente deben ser configurados para manejar la conexión a través de un **agente de retransmisión DHCP**. Esto implica lo siguiente:
- **Cliente:** El cliente debe estar configurado para enviar solicitudes de DHCPDISCOVER a un agente de retransmisión remoto, que posteriormente redirige la solicitud al servidor DHCP.
- **Servidor:** El servidor debe ser capaz de aceptar solicitudes que provienen de un agente de retransmisión, procesarlas como si provinieran directamente del cliente, y devolver la respuesta a través del agente de retransmisión.

El **agente de retransmisión DHCP** es necesario para conectar clientes que se encuentran en subredes diferentes al servidor DHCP. Este agente debe reenviar paquetes DHCP entre el cliente y el servidor, asegurándose de que las respuestas lleguen correctamente al cliente a través de la red remota.

## 3. Requisitos del Cliente DHCP

El cliente debe cumplir con los siguientes requisitos:

### Requisitos Funcionales:
1. **Elección de lenguaje:** El cliente puede ser desarrollado en cualquier lenguaje de programación que soporte la API de Sockets (por ejemplo, Python, Java, C).
2. **Manejo de mensajes DHCP:** El cliente debe manejar las fases principales del proceso DHCP:
   - **DHCPDISCOVER:** Enviar un mensaje **DHCPDISCOVER** al servidor para solicitar una dirección IP.
   - **DHCPOFFER:** Recibir una oferta de dirección IP del servidor mediante un mensaje **DHCPOFFER**.
   - **DHCPREQUEST:** Confirmar el interés en arrendar la IP ofrecida enviando un mensaje **DHCPREQUEST** al servidor.
   - **DHCPACK:** Recibir la confirmación final de la asignación de la IP por parte del servidor mediante un mensaje **DHCPACK**.
   - (Opcional) **DHCPNAK:** Manejar un **DHCPNAK** si el servidor no puede conceder la IP solicitada.
   - (Opcional) **DHCPDECLINE:** Enviar un **DHCPDECLINE** al servidor si el cliente detecta que la IP ofrecida está en uso.
   - **DHCPRELEASE:** Enviar un mensaje **DHCPRELEASE** al servidor para liberar la IP asignada cuando ya no sea necesaria.
3. **Mostrar la configuración de red:** El cliente debe mostrar las configuraciones de red asignadas después de recibir la IP, como:
   - Dirección IP
   - Máscara de subred
   - Puerta de enlace predeterminada
   - Servidor DNS
   - Duración del arrendamiento (si está disponible)
4. **Renovación de dirección IP:** El cliente debe ser capaz de renovar la dirección IP antes de que expire enviando un mensaje **DHCPREQUEST** al servidor.
5. **Liberación de dirección IP:** El cliente debe liberar la IP cuando ya no la necesite enviando un mensaje **DHCPRELEASE**.
6. **Manejo de errores:** El cliente debe manejar errores como no recibir una respuesta dentro de un tiempo límite o recibir mensajes incorrectos del servidor.

## 4. Requisitos del Servidor DHCP

El servidor debe cumplir con los siguientes requisitos:

### Requisitos Funcionales:
1. **Restricción de lenguaje:** El servidor debe implementarse en C, utilizando la API de Sockets Berkeley.
2. **Escuchar solicitudes DHCP:** El servidor debe escuchar en el puerto 67 para recibir mensajes **DHCPDISCOVER** de los clientes.
3. **Asignación dinámica de direcciones IP:** El servidor debe gestionar un pool de direcciones IP disponibles para asignarlas a los clientes.
4. **Manejo de mensajes DHCP:** El servidor debe manejar las fases principales del proceso DHCP:
   - **DHCPDISCOVER:** Responder al cliente con un mensaje **DHCPOFFER**, ofreciendo una IP.
   - **DHCPREQUEST:** Confirmar la asignación de la IP enviando un **DHCPACK** al cliente.
   - (Opcional) **DHCPNAK:** Enviar un **DHCPNAK** si no se puede conceder la IP solicitada.
   - (Opcional) **DHCPDECLINE:** Manejar casos en los que el cliente rechace una IP ofrecida.
   - **DHCPRELEASE:** Procesar mensajes de liberación de IP por parte del cliente para devolver la IP al pool.
5. **Gestión de arrendamientos:** El servidor debe rastrear la duración del arrendamiento de cada dirección IP que asigne, permitiendo a los clientes renovar sus arrendamientos o liberar las direcciones IP.
6. **Concurrencia:** El servidor debe ser capaz de manejar múltiples solicitudes de clientes simultáneamente utilizando multithreading.
7. **Registro de eventos:** El servidor debe registrar cada evento de asignación, renovación, liberación y errores para facilitar la depuración y mejorar la eficiencia del sistema, así como llevar una monitoria de a qué equipo se asignó cada IP en el tiempo.

## 5. Visión General de la Comunicación Cliente-Servidor

Tanto el **cliente** como el **servidor** deben comunicarse utilizando **UDP** como protocolo de transporte. Los puertos estándar utilizados son:
- **Puerto del servidor DHCP:** 67
- **Puerto del cliente DHCP:** 68

El **cliente** inicia la comunicación enviando un mensaje de difusión (**DHCPDISCOVER**) a la red. El servidor responde con un mensaje **DHCPOFFER** que contiene una dirección IP disponible. El cliente luego solicita la IP mediante un mensaje **DHCPREQUEST**, y el servidor la confirma con un mensaje **DHCPACK**. Ambas aplicaciones deben manejar correctamente esta secuencia.

### Flujo de Mensajes:
1. **DHCPDISCOVER:** Cliente -> Servidor (Solicitud de dirección IP)
2. **DHCPOFFER:** Servidor -> Cliente (Oferta de dirección IP)
3. **DHCPREQUEST:** Cliente -> Servidor (Solicitud de arrendar la IP)
4. **DHCPACK:** Servidor -> Cliente (Confirmación del arrendamiento)

(Opcionales):
- **DHCPRELEASE:** Cliente -> Servidor (Liberación de la IP)
- **DHCPDECLINE:** Cliente -> Servidor (Rechazo de la IP)
- **DHCPNAK:** Servidor -> Cliente (Negativa del servidor ante un error en la solicitud)
