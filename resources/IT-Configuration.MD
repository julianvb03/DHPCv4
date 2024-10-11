# Infraestructura

El problema que vamos a abordar se puede expresar con la topoliga de red expresada en la figura 1, en la cual podemos ver como hay 2 subredes conectadas a un router el cual permite la transmisión de los mensajes de esta, en la VLAN 100 se encuentra un cliente y el servidor, por otro lado en la VLAN 101 hay unicamente un servidor. 

El objetivo es que ambos clientes puedan solicitar direcciones ip validas para sus respectivas subredes 192.168.1.x y 192.168.2.x, de lo cual se encargara el servidor dchp.

## Configuración de las subredes
1. **Configuración de los switches:** El primer paso consta de la configuración de los diferentes switches para que puedan reconocer el trafico en sus respectivas VLAN, usando un **Type** ***accses*** en su enlace con cada maquina y ***dot1q*** en el puerto que conecta con el switch, permitiendo que el tráfico de múltiples VLANs sea enviado a través de un solo enlace (un enlace ***trunk***). La configuración será de esta forma (los puertos elegidos son arbitrarios) y la VLAN en los puertos que usan el tipo **dot1q** tienen el valor de 1 ya que la VLAN 1 es especial porque es la VLAN nativa predeterminada para manejar tráfico no etiquetado y se usa para la gestión y control de red (como CDP y otros protocolos de administración). Esto la hace fundamental para la interoperabilidad y la configuración inicial de dispositivos de red..

    | Port | VLAN | Type | Descripción |
    |---------|-------------|-------------|-------------|
    | `0` | 1 | dot1q | Configuración del puerto que conecta el swith con el router |
    | `1` | 100 | accsess | Configuración del puerto que conecta el swith con el servidor |
    | `2` | 100 | accsess | Configuración del puerto que conecta el swith con el cliente #1 |
    | `0` | 1 | dot1q | Configuración del puerto que conecta el swith #2 con el router |
    | `1` | 100 | accsess | Configuración del puerto que conecta el swith con el cliente #2 |    
    
    
2. **Configuración del router:** Tenemos que configurar nuestro router para que pueda manejar correctamente las diferentes subredes, para ello dentro de la terminal del router seguiremos los siguientes pasos.
   1. Accederemos al modo de configuracion con `configure terminal`.
   2. Accederemos a nuestra primera interfaz, en el caso de mi router y slot predeterminado sería con el comando `interface FastEthernet0/0`, posteriormente encenderemos la interfaz con `no shutdown`, y después pondríamos el comando `end` y para finalizar (este proceso lo repetimos con la otra interfaz en mi caso la `interface FastEthernet0/1`) .
   3. Seguiremos con la configuración de nuestras subinterfaces `interface FastEthernet 0/0.100` (en donde el ***.100*** por buenas practicas hacer referencia a mi VLAN), proceguiremos encendiendo nuestra interface con `no shutdown`, continuando con esta configuración de subred especificaremos con el comando `encapsulation dot1Q 100` que la VLAN que asignamos como **100**, esto permite el correcto enrrutamiento entre VLANs y para finalizar asignaremos la ip del default gateway con el comando `ip addres 192.168.1.1 255.255.255.0` para asignar la ip de dafault gateway la mascara de subred.
   4. Para finalizar repetiremos lo anterior pero para la otra interface, en mi caso particular VLAN `interface FastEthernet 0/0.101` con una gateway establecida por `ip addres 192.168.2.1 255.255.255.0`
3. Third item

| Comando | Descripción |
|---------|-------------|
| `show ip interface brief` | Muestra un resumen del estado de las interfaces. |
| `show interfaces FastEthernet0/0` | Muestra detalles completos de una interfaz. |
| `show running-config interface FastEthernet0/0` | Muestra la configuración actual de una interfaz específica. |
| `configure terminal` | Entra al modo de configuración global. |
| `interface FastEthernet0/0` | Entra a la configuración de una interfaz específica. |
| `ip address 192.168.1.1 255.255.255.0` | Asigna una dirección IP a la interfaz. |
| `no shutdown` | Activa la interfaz. |
| `shutdown` | Desactiva la interfaz. |
| `speed 100` | Configura la interfaz para funcionar a 100 Mbps. |
| `duplex full` | Configura la interfaz para funcionar en dúplex completo. |
| `description Conexión a la red LAN principal` | Agrega una descripción a la interfaz. |
| `copy running-config startup-config` | Guarda la configuración actual para que se mantenga tras un reinicio. |
| `show startup-config` | Muestra la configuración que se cargará en el próximo reinicio. |
| `show interfaces FastEthernet0/0 include packets` | Filtra para mostrar solo las líneas relacionadas con paquetes transmitidos y recibidos. |
| `show interfaces FastEthernet0/0 include error` | Filtra para mostrar solo los errores de la interfaz. |


| Comando | Función |
|---------|---------|
| `enable` | Permite acceder al modo privilegiado, donde se pueden ejecutar comandos de administración avanzados. |
| `configure terminal` | Entra al modo de configuración global para realizar cambios en la configuración del dispositivo. |
| `interface <interfaz>` | Selecciona una interfaz física o subinterfaz específica para configurar (`<interfaz>` puede ser, por ejemplo, `FastEthernet0/0` o `GigabitEthernet0/1`). |
| `no shutdown` | Habilita o activa una interfaz que está deshabilitada por defecto, permitiendo que inicie la comunicación de red. |
| `encapsulation dot1Q <VLAN_ID>` | Configura una subinterfaz para manejar tráfico etiquetado de una VLAN específica usando el estándar 802.1Q. `<VLAN_ID>` es el identificador de la VLAN, como `100`, `102`, etc. |
| `ip address <IP> <máscara>` | Asigna una dirección IP a una interfaz o subinterfaz, con `<IP>` como la dirección IP (por ejemplo, `192.168.1.1`) y `<máscara>` como la máscara de subred (por ejemplo, `255.255.255.0`). |
| `encapsulation dot1Q <VLAN_ID> native` | Configura la subinterfaz para manejar el tráfico sin etiquetar, asignándolo a la VLAN nativa especificada (`<VLAN_ID>`). |
| `end` | Sale del modo de configuración y vuelve al modo privilegiado. |
| `copy running-config startup-config` | Guarda la configuración actual para que persista después de un reinicio del dispositivo. |

