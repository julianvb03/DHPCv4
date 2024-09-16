# Resolución de direcciones

## ARP

- Cuando un paquete es enviado a nivel de red, este va encapsulado en una trama del nivel de enlace. El emisor debe saber qué dirección poner en la trama para la correcta comunicación, y esta casi siempre es la MAC, para eso se utiliza ARP.
- Los mensajes ARP no son paquetes IP, por lo que tienen un Ethertype diferente: **0x0800** en el caso de IP y **0x0806** en el caso de ARP.

## Caso local

- Cuando un equipo en la red local quiere hacer un ping, por ejemplo, primero se crea el paquete IP. Luego se hace una operación AND bit a bit con la máscara de red para verificar si el dispositivo con esa IP está en la misma subred. Si es así, se encapsula en una trama Ethernet. Para hacer esto, se necesita la dirección MAC, y aquí es donde entra ARP.
- Se construye una tabla en el host que hace la correspondencia de IPs a MACs (ARP Cache), que inicia vacía. Se manda un ARP Request a una dirección de broadcast preguntando por el destinatario. Uno de los dispositivos en la red responderá (el que tenga dicha IP). El dispositivo que responde guarda la IP y MAC de quien preguntó en su ARP Cache. Posteriormente, el dispositivo con esa IP responderá con un ARP Reply de forma unicast, ya que ya conoce los datos del solicitante. Al recibir esto, el solicitante anotará en su ARP Cache la información de la máquina por la cual preguntó.
- En algunos casos, otros dispositivos en la red también pueden actualizar su ARP Cache al recibir el ARP Reply.

## Caso remoto

- Si al hacer la comparación de la máscara con la dirección IP de destino se determina que no está en la red local, no se preguntará por la IP del dispositivo destino, sino por la del **default gateway**, que será el router a quien se enviará el mensaje para que lo reenvíe.
- El router guardará en su ARP Cache la IP y MAC del solicitante y enviará un ARP Reply. El solicitante guardará en su ARP Cache la IP y MAC del router, y entonces enviará el ping a la dirección de destino a través del router. El router enviará el paquete a Internet y reenviará la respuesta al solicitante.
- En este proceso, el dispositivo que hizo el ping usará la IP del destino externo pero la dirección MAC del router, que está en su ARP Cache. La dirección MAC cambiará a medida que el paquete pasa por routers en redes intermedias.

- En el mensaje ARP se repite la información de la MAC de origen y de destino (aunque ya están en la trama Ethernet), porque el modelo de capas dice que cada capa debe ser autosuficiente.

## Tabla ARP Cache

- La tabla ARP Cache elimina una entrada si no se usa en 15 a 20 minutos, dependiendo de la implementación, lo que hace que sea necesario rellenarla de nuevo si hay una nueva comunicación. Esta tabla se indexa por IPs, lo que significa que varias IPs pueden apuntar a la misma MAC, pero no puede haber una IP que apunte a más de una MAC diferente.
- Si se recibe por ARP una nueva MAC para una IP existente, se sustituye inmediatamente la vieja MAC por la nueva.
- Es importante tener en cuenta que el tamaño de la tabla ARP puede estar limitado por la configuración del sistema operativo, lo que podría causar problemas en redes con muchos dispositivos.

## Resolución inversa de direcciones

- La resolución inversa de direcciones es lo opuesto a ARP (Address Resolution Protocol). ARP toma una dirección IP y devuelve la MAC asociada, mientras que RARP (Reverse Address Resolution Protocol) cumple el papel opuesto: conociendo la dirección MAC, se obtiene una dirección IP.
- RARP se apoya en un **servidor** que almacena las correspondencias MAC → IP, y hay tres protocolos que ayudan en esto: **RARP**, **BOOTP** y **DHCP**.

## RARP

> 💡 Protocolo obsoleto, por eso no se hablará mucho de él.

- El formato de mensaje es el mismo que ARP.
- Para consultar una IP, se hace un RARP Request en modo broadcast y se recibe un RARP Reply si el servidor encuentra la MAC en sus tablas, en modo unicast.
- Utiliza un Ethertype diferente a ARP para facilitar el filtrado y distinguir un ARP Request de un RARP.
- Problemas:
    - Solo devuelve IPs y no configura máscaras de red, lo que limita su utilidad.
    - No es enrutable, quedando confinado en una red local (igual que ARP).

## BOOTP

- Su nombre proviene de la necesidad de usar máquinas sin disco (diskless).
- BOOTP pide a un servidor toda la configuración IP, incluidas máscaras de subred.
- Como es un protocolo IP, puede viajar a través de routers, por lo que el servidor no necesita estar en la red local.
- Un **agente** en la red local (generalmente el router) recibe la petición BOOTP Request en modo broadcast y la reenvía de forma unicast al servidor BOOTP, que conoce la dirección del servidor.

### BOOTP sin modificar ARP Cache

- El cliente envía un BOOTP Request broadcast a la dirección 255.255.255.255 con la IP de origen 0.0.0.0 (ya que no conoce su IP).
- El servidor responde en broadcast, ya que el cliente no tiene una entrada ARP para ser identificado.

### BOOTP puede modificar ARP Cache

- Si el kernel lo permite, BOOTP puede modificar la ARP Cache añadiendo la MAC y la IP del cliente, y puede enviar un BOOTP Reply unicast.

### BOOTP servidor remoto

- La LAN envía la petición BOOTP en broadcast. El agente (router) reenvía la petición de forma unicast al servidor remoto, y recibe la respuesta en unicast. Dependiendo del agente, la respuesta puede enviarse en broadcast o unicast.

## DHCP

- DHCP es similar a BOOTP pero con diferencias en la asignación de IPs. En DHCP hay tres opciones:
    - **Configuración estática:** Igual que BOOTP, IP manual.
    - **Configuración dinámica:** Las IPs se alquilan por un periodo de tiempo y deben renovarse.
    - **Asignación automática:** Una IP es asignada permanentemente a una MAC solicitante.
- Los paquetes DHCP y BOOTP son idénticos. Las respuestas pueden ser broadcast o unicast, y un servidor DHCP que usa direccionamiento manual es equivalente a uno BOOTP.
- Es común dejar un subconjunto de rango de IPs reservado para configuraciones manuales.

## Consideraciones de seguridad

- ARP es susceptible a ataques como **ARP spoofing**, en los que un atacante engaña a los dispositivos de la red para asociar su dirección MAC con la IP de otro dispositivo. Esto puede permitir ataques de tipo Man-in-the-Middle (MITM). Es importante implementar medidas de seguridad como la autenticación de ARP o tablas ARP estáticas en redes sensibles.
